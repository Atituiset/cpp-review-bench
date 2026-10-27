# auto-redis-0de2a7a592

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#16074](https://github.com/redis/redis/pull/16074) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 67 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16074 (https://github.com/redis/redis/pull/16074)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 3869；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16074 Redis 8.6.8 :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3850,6 +3850,26 @@ void clusterBroadcastPong(int target) {
     dictResetIterator(&di);
 }
 
+/* Compute the length of a PUBLISH message into '*msglen'. Returns C_ERR if
+ * the channel and message are too large to fit in a cluster bus message. */
+int clusterGetPublishMessageLength(robj *channel, robj *message, uint32_t *msglen) {
+    size_t channel_len = stringObjectLen(channel);
+    size_t message_len = stringObjectLen(message);
+    size_t base_len = sizeof(clusterMsg)-sizeof(union clusterMsgData) +
+                      sizeof(clusterMsgDataPublish) - 8;
+    size_t max_msglen = UINT32_MAX - sizeof(clusterMsgSendBlock);
+
+    if (channel_len > UINT32_MAX || message_len > UINT32_MAX ||
+        channel_len > max_msglen - base_len ||
+        message_len > max_msglen - base_len - channel_len)
+    {
+        return C_ERR;
+    }
+
+    *msglen = base_len + channel_len + message_len;
+    return C_OK;
+}
+
 /* Create a PUBLISH message block.
  *
  * Sanitizer suppression: In clusterMsgDataPublish, sizeof(bulk_data) is 8.
@@ -3860,14 +3880,16 @@ REDIS_NO_SANITIZE("bounds")
 clusterMsgSendBlock *clusterCreatePublishMsgBlock(robj *channel, robj *message, uint16_t type) {
 
     uint32_t channel_len, message_len;
+    uint32_t msglen;
+
+    if (clusterGetPublishMessageLength(channel, message, &msglen) == C_ERR)
+        return NULL;
 
     channel = getDecodedObject(channel);
     message = getDecodedObject(message);
     channel_len = sdslen(channel->ptr);
     message_len = sdslen(message->ptr);
 
-    size_t msglen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
-    msglen += sizeof(clusterMsgDataPublish) - 8 + channel_len + message_len;
     clusterMsgSendBlock *msgblock = createClusterMsgSendBlock(type, msglen);
 
     clusterMsg *hdr = getMessageFromSendBlock(msgblock);
@@ -3980,6 +4002,7 @@ void clusterPropagatePublish(robj *channel, robj *message, int sharded) {
 
     if (!sharded) {
         msgblock = clusterCreatePublishMsgBlock(channel, message, CLUSTERMSG_TYPE_PUBLISH);
+        if (msgblock == NULL) return;
         clusterBroadcastMessage(msgblock);
         clusterMsgSendBlockDecrRefCount(msgblock);
         return;
@@ -3991,6 +4014,7 @@ void clusterPropagatePublish(robj *channel, robj *message, int sharded) {
     serverAssert(nodes_for_slot != NULL);
     listRewind(nodes_for_slot, &li);
     msgblock = clusterCreatePublishMsgBlock(channel, message, CLUSTERMSG_TYPE_PUBLISHSHARD);
+    if (msgblock == NULL) return;
     while((ln = listNext(&li))) {
         clusterNode *node = listNodeValue(ln);
         if (node->flags & (CLUSTER_NODE_MYSELF|CLUSTER_NODE_HANDSHAKE))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clusterCreatePublishMsgBlock`
- 外部函数：`clusterNodeIsMaster`
- 外部函数：`connClose`
- 外部函数：`connGetLastError`
- 外部函数：`connGetPrivateData`
- 外部函数：`connSetWriteHandler`
- 外部函数：`connSetWriteHandlerWithBarrier`
- 外部函数：`connWrite`
- 外部函数：`dictGetVal`
- 外部函数：`dictInitSafeIterator`
- 外部函数：`dictNext`
- 外部函数：`dictResetIterator`
- 外部函数：`getDecodedObject`
- 外部函数：`htonl`
- 外部函数：`htons`
- 外部函数：`htonu64`
- 外部函数：`instead`
- 外部函数：`listAddNodeTail`
- 外部函数：`listDelNode`
- 外部函数：`listFirst`
- 外部函数：`listLength`
- 外部函数：`listNext`
- 外部函数：`listNodeValue`
- 外部函数：`listRelease`
- 外部函数：`listRewind`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`nodeIsSlave`
- 外部函数：`ntohl`
- 外部函数：`ntohs`
- 外部函数：`redis_strlcpy`
- 外部函数：`replicationGetSlaveOffset`
- 外部函数：`sdslen`
- 外部函数：`serverAssert`
- 外部函数：`serverLog`
- 外部函数：`zcalloc`
- 外部函数：`zfree`
- 大写宏：`CLUSTERMSG_FLAG0_EXT_DATA`
- 大写宏：`CLUSTERMSG_FLAG0_PAUSED`
- 大写宏：`CLUSTERMSG_TYPE_COUNT`
- 大写宏：`CLUSTERMSG_TYPE_PUBLISH`
- 大写宏：`CLUSTERMSG_TYPE_PUBLISHSHARD`
- 大写宏：`CLUSTER_NAMELEN`
- 大写宏：`CLUSTER_NODE_HANDSHAKE`
- 大写宏：`CLUSTER_NODE_MASTER`
- 大写宏：`CLUSTER_NODE_MYSELF`
- 大写宏：`CLUSTER_PORT_INCR`
- 大写宏：`CLUSTER_PROTO_VER`
- 大写宏：`LL_DEBUG`
- 大写宏：`NET_IP_STR_LEN`
- 大写宏：`NET_MAX_WRITES_PER_EVENT`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PUBLISH`
- 外部类型：`Always`
- 外部类型：`Config`
- 外部类型：`Create`
- 外部类型：`Delete`
- 外部类型：`Derive`
- 外部类型：`Handle`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Number`
- 外部类型：`Otherwise`
- 外部类型：`Populate`
- 外部类型：`Sanitizer`
- 外部类型：`Set`
- 外部类型：`Total`
- 外部类型：`Update`
- 外部类型：`clusterMsgData`
- 外部类型：`size_t`
- 外部类型：`ssize_t`
- 外部类型：`uint16_t`
- 外部类型：`uint32_t`
- 外部类型：`uint64_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-redis-0de2a7a592` → 本草稿移入 `cases/defect/auto-redis-0de2a7a592/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
