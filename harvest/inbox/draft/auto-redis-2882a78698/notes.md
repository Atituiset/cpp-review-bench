# auto-redis-2882a78698

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15187 (https://github.com/redis/redis/pull/15187)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2843；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2839,11 +2839,16 @@ int clusterProcessPacket(clusterLink *link) {
         explen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
         explen += sizeof(clusterMsgDataFail);
     } else if (type == CLUSTERMSG_TYPE_PUBLISH || type == CLUSTERMSG_TYPE_PUBLISHSHARD) {
+        uint32_t ch_len = ntohl(hdr->data.publish.msg.channel_len);
+        uint32_t msg_len = ntohl(hdr->data.publish.msg.message_len);
         explen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
-        explen += sizeof(clusterMsgDataPublish) -
-                8 +
-                ntohl(hdr->data.publish.msg.channel_len) +
-                ntohl(hdr->data.publish.msg.message_len);
+        explen += sizeof(clusterMsgDataPublish) - 8;
+        if (ch_len > UINT32_MAX - explen || msg_len > UINT32_MAX - explen - ch_len) {
+            serverLog(LL_WARNING, "Received invalid %s packet with overflow in length fields "
+                "(channel_len:%u, message_len:%u)", clusterGetMessageTypeString(type), ch_len, msg_len);
+            return 1;
+        }
+        explen += ch_len + msg_len;
     } else if (type == CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST ||
                type == CLUSTERMSG_TYPE_FAILOVER_AUTH_ACK ||
                type == CLUSTERMSG_TYPE_MFSTART)
@@ -2853,9 +2858,15 @@ int clusterProcessPacket(clusterLink *link) {
         explen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
         explen += sizeof(clusterMsgDataUpdate);
     } else if (type == CLUSTERMSG_TYPE_MODULE) {
+        uint32_t module_len = ntohl(hdr->data.module.msg.len);
         explen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
-        explen += sizeof(clusterMsgModule) -
-                3 + ntohl(hdr->data.module.msg.len);
+        explen += sizeof(clusterMsgModule) - 3;
+        if (module_len > UINT32_MAX - explen) {
+            serverLog(LL_WARNING, "Received invalid %s packet with overflow in length field "
+                "(len:%u)", clusterGetMessageTypeString(type), module_len);
+            return 1;
+        }
+        explen += module_len;
     } else {
         /* We don't know this type of packet, so we assume it's well formed. */
         explen = totlen;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-2882a78698` → 本草稿移入 `cases/defect/auto-redis-2882a78698/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
