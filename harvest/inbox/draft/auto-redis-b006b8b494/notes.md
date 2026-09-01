# auto-redis-b006b8b494

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#13637](https://github.com/redis/redis/pull/13637) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 76 |
| 编译错误数（gcc syntax-only） | 43（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #13637 (https://github.com/redis/redis/pull/13637)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 5659；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 13637 Release 7.4.2 :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2,8 +2,13 @@
  * Copyright (c) 2009-Present, Redis Ltd.
  * All rights reserved.
  *
+ * Copyright (c) 2024-present, Valkey contributors.
+ * All rights reserved.
+ *
  * Licensed under your choice of the Redis Source Available License 2.0
  * (RSALv2) or the Server Side Public License v1 (SSPLv1).
+ *
+ * Portions of this file are available under BSD3 terms; see REDISCONTRIBUTIONS for more information.
  */
 
 /*
@@ -634,6 +639,8 @@ int clusterLoadConfig(char *filename) {
     }
     /* Config sanity check */
     if (server.cluster->myself == NULL) goto fmterr;
+    if (!(myself->flags & (CLUSTER_NODE_MASTER | CLUSTER_NODE_SLAVE))) goto fmterr;
+    if (nodeIsSlave(myself) && myself->slaveof == NULL) goto fmterr;
 
     zfree(line);
     fclose(fp);
@@ -1624,6 +1631,22 @@ void clusterRemoveNodeFromShard(clusterNode *node) {
     sdsfree(s);
 }
 
+static clusterNode *clusterGetMasterFromShard(list *nodes) {
+    clusterNode *n = NULL;
+    listIter li;
+    listNode *ln;
+    listRewind(nodes,&li);
+    while ((ln = listNext(&li)) != NULL) {
+        clusterNode *node = listNodeValue(ln);
+        if (!nodeFailed(node)) {
+            n = node;
+            break;
+        }
+    }
+    if (!n) return NULL;
+    return clusterNodeGetMaster(n);
+}
+
 /* -----------------------------------------------------------------------------
  * CLUSTER config epoch handling
  * -------------------------------------------------------------------------- */
@@ -2577,9 +2600,6 @@ uint32_t writePingExt(clusterMsg *hdr, int gossipcount)  {
     extensions++;
 
     if (hdr != NULL) {
-        if (extensions != 0) {
-            hdr->mflags[0] |= CLUSTERMSG_FLAG0_EXT_DATA;
-        }
         hdr->extensions = htons(extensions);
     }
 
@@ -2769,6 +2789,9 @@ int clusterProcessPacket(clusterLink *link) {
     }
 
     sender = getNodeFromLinkAndMsg(link, hdr);
+    if (sender && (hdr->mflags[0] & CLUSTERMSG_FLAG0_EXT_DATA)) {
+        sender->flags |= CLUSTER_NODE_EXTENSIONS_SUPPORTED;
+    }
 
     /* Update the last time we saw any data from this node. We
      * use this in order to avoid detecting a timeout from a node that
@@ -3534,6 +3557,8 @@ static void clusterBuildMessageHdr(clusterMsg *hdr, int type, size_t msglen) {
     /* Set the message flags. */
     if (clusterNodeIsMaster(myself) && server.cluster->mf_end)
         hdr->mflags[0] |= CLUSTERMSG_FLAG0_PAUSED;
+    hdr->mflags[0] |= CLUSTERMSG_FLAG0_EXT_DATA; /* Always make other nodes know that
+                                                  * this node supports extension data. */
 
     hdr->totlen = htonl(msglen);
 }
@@ -3612,7 +3637,9 @@ void clusterSendPing(clusterLink *link, int type) {
      * to put inside the packet. */
     estlen = sizeof(clusterMsg) - sizeof(union clusterMsgData);
     estlen += (sizeof(clusterMsgDataGossip)*(wanted + pfail_wanted));
-    estlen += writePingExt(NULL, 0);
+    if (link->node && nodeSupportsExtensions(link->node)) {
+        estlen += writePingExt(NULL, 0);
+    }
     /* Note: clusterBuildMessageHdr() expects the buffer to be always at least
      * sizeof(clusterMsg) or more. */
     if (estlen < (int)sizeof(clusterMsg)) estlen = sizeof(clusterMsg);
@@ -3682,7 +3709,9 @@ void clusterSendPing(clusterLink *link, int type) {
 
     /* Compute the actual total length and send! */
     uint32_t totlen = 0;
-    totlen += writePingExt(hdr, gossipcount);
+    if (link->node && nodeSupportsExtensions(link->node)) {
+        totlen += writePingExt(hdr, gossipcount);
+    }
     totlen += sizeof(clusterMsg)-sizeof(union clusterMsgData);
     totlen += (sizeof(clusterMsgDataGossip)*gossipcount);
     serverAssert(gossipcount < USHRT_MAX);
@@ -5649,14 +5678,13 @@ void addNodeDetailsToShardReply(client *c, clusterNode *node) {
 /* Add the shard reply of a single shard based off the given primary node. */
 void addShardReplyForClusterShards(client *c, list *nodes) {
     serverAssert(listLength(nodes) > 0);
-    clusterNode *n = listNodeValue
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`addReplyArrayLen`
- 外部函数：`addReplyBulkCString`
- 外部函数：`addReplyMapLen`
- 外部函数：`clusterBuildMessageHdr`
- 外部函数：`connClose`
- 外部函数：`dictFind`
- 外部函数：`dictGetIterator`
- 外部函数：`dictGetKey`
- 外部函数：`dictGetUnsignedIntegerVal`
- 外部函数：`dictGetVal`
- 外部函数：`dictNext`
- 外部函数：`dictReleaseIterator`
- 外部函数：`dictSize`
- 外部函数：`fclose`
- 外部函数：`htonl`
- 外部函数：`htons`
- 外部函数：`htonu64`
- 外部函数：`listFirst`
- 外部函数：`listLength`
- 外部函数：`listNodeValue`
- 外部函数：`listRelease`
- 外部函数：`memcpy`
- 外部函数：`nodeInHandshake`
- 外部函数：`ntohl`
- 外部函数：`sdsfree`
- 外部函数：`sdslen`
- 外部函数：`sdsnewlen`
- 外部函数：`serverAssert`
- 外部函数：`serverLog`
- 外部函数：`v1`
- 外部函数：`verifyClusterNodeId`
- 外部函数：`zfree`
- 大写宏：`CLUSTER`
- 大写宏：`CLUSTERMSG_EXT_TYPE_FORGOTTEN_NODE`
- 大写宏：`CLUSTERMSG_EXT_TYPE_HOSTNAME`
- 大写宏：`CLUSTERMSG_EXT_TYPE_HUMAN_NODENAME`
- 大写宏：`CLUSTERMSG_EXT_TYPE_SHARDID`
- 大写宏：`CLUSTERMSG_FLAG0_EXT_DATA`
- 大写宏：`CLUSTERMSG_FLAG0_PAUSED`
- 大写宏：`CLUSTER_NAMELEN`
- 大写宏：`CLUSTER_NODE_MASTER`
- 大写宏：`C_OK`
- 大写宏：`EIGHT_BYTE_ALIGN`
- 大写宏：`LL_DEBUG`
- 大写宏：`NULL`
- 大写宏：`USHRT_MAX`
- 外部类型：`Add`
- 外部类型：`All`
- 外部类型：`Available`
- 外部类型：`Compute`
- 外部类型：`Config`
- 外部类型：`Gossip`
- 外部类型：`If`
- 外部类型：`License`
- 外部类型：`Licensed`
- 外部类型：`Ltd`
- 外部类型：`Move`
- 外部类型：`Note`
- 外部类型：`Otherwise`
- 外部类型：`Our`
- 外部类型：`Populate`
- 外部类型：`Present`
- 外部类型：`Public`
- 外部类型：`RSALv2`
- 外部类型：`Redis`
- 外部类型：`Replacing`
- 外部类型：`SSPLv1`
- 外部类型：`Server`
- 外部类型：`Set`
- 外部类型：`Side`
- 外部类型：`Source`
- 外部类型：`The`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`Update`
- 外部类型：`Use`
- 外部类型：`We`
- 外部类型：`clusterMsgData`
- 外部类型：`time_t`
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

1. 完成上面检查清单后评论 `/case accept auto-redis-b006b8b494` → 本草稿移入 `cases/defect/auto-redis-b006b8b494/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
