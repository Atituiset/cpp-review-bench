# auto-redis-f6dc049963

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#16074](https://github.com/redis/redis/pull/16074) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 31 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16074 (https://github.com/redis/redis/pull/16074)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16074 Redis 8.6.8 :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -610,6 +610,12 @@ void punsubscribeCommand(client *c) {
 /* This function wraps pubsubPublishMessage and also propagates the message to cluster.
  * Used by the commands PUBLISH/SPUBLISH and their respective module APIs.*/
 int pubsubPublishMessageAndPropagateToCluster(robj *channel, robj *message, int sharded) {
+    if (server.cluster_enabled) {
+        uint32_t msglen;
+        if (clusterGetPublishMessageLength(channel, message, &msglen) == C_ERR)
+            return C_ERR;
+    }
+
     int receivers = pubsubPublishMessage(channel, message, sharded);
     if (server.cluster_enabled)
         clusterPropagatePublish(channel, message, sharded);
@@ -624,6 +630,10 @@ void publishCommand(client *c) {
     }
 
     int receivers = pubsubPublishMessageAndPropagateToCluster(c->argv[1],c->argv[2],0);
+    if (receivers == C_ERR) {
+        addReplyError(c,"Publish message is too large for cluster propagation");
+        return;
+    }
     if (!server.cluster_enabled)
         forceCommandPropagation(c,PROPAGATE_REPL);
     addReplyLongLong(c,receivers);
@@ -720,6 +730,10 @@ void channelList(client *c, sds pat, kvstore *pubsub_channels) {
 /* SPUBLISH <shardchannel> <message> */
 void spublishCommand(client *c) {
     int receivers = pubsubPublishMessageAndPropagateToCluster(c->argv[1],c->argv[2],1);
+    if (receivers == C_ERR) {
+        addReplyError(c,"Publish message is too large for cluster propagation");
+        return;
+    }
     if (!server.cluster_enabled)
         forceCommandPropagation(c,PROPAGATE_REPL);
     addReplyLongLong(c,receivers);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`addReply`
- 外部函数：`addReplyBulk`
- 外部函数：`addReplyLongLong`
- 外部函数：`addReplyPushLen`
- 外部函数：`clusterPropagatePublish`
- 外部函数：`clusterSlotStatsAddNetworkBytesOutForShardedPubSubInternalPropagation`
- 外部函数：`clusterSlotStatsEnabled`
- 外部函数：`decrRefCount`
- 外部函数：`dictGetKey`
- 外部函数：`dictGetVal`
- 外部函数：`dictInitIterator`
- 外部函数：`dictNext`
- 外部函数：`dictResetIterator`
- 外部函数：`dictSize`
- 外部函数：`forceCommandPropagation`
- 外部函数：`getDecodedObject`
- 外部函数：`int`
- 外部函数：`keyHashSlot`
- 外部函数：`kvstoreDictFind`
- 外部函数：`sdslen`
- 外部函数：`stringmatchlen`
- 外部函数：`updateClientMemUsageAndBucket`
- 大写宏：`CLIENT_PUSHING`
- 大写宏：`CLUSTER_SLOT_STATS_NET`
- 大写宏：`NULL`
- 大写宏：`PROPAGATE_REPL`
- 大写宏：`PUBLISH`
- 大写宏：`SPUBLISH`
- 外部类型：`APIs`
- 外部类型：`Send`
- 外部类型：`Shard`
- 外部类型：`This`
- 外部类型：`Used`
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

1. 完成上面检查清单后评论 `/case accept auto-redis-f6dc049963` → 本草稿移入 `cases/defect/auto-redis-f6dc049963/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
