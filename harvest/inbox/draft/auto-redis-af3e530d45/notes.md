# auto-redis-af3e530d45

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14750 (https://github.com/redis/redis/pull/14750)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 240；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -237,7 +237,11 @@ int blockedClientMayTimeout(client *c) {
  * unblockClient() will be called with the same client as argument. */
 void replyToBlockedClientTimedOut(client *c) {
     if (c->bstate.btype == BLOCKED_LAZYFREE) {
-        addReply(c, shared.ok); /* No reason lazy-free to fail */
+        /* SFLUSH: reply with empty array, FLUSH*: reply with OK */
+        if (c->cmd && c->cmd->proc == sflushCommand)
+            addReplyArrayLen(c, 0);
+        else
+            addReply(c, shared.ok); /* No reason lazy-free to fail */
     } else if (c->bstate.btype == BLOCKED_LIST ||
         c->bstate.btype == BLOCKED_ZSET ||
         c->bstate.btype == BLOCKED_STREAM) {
@@ -297,7 +301,11 @@ void disconnectAllBlockedClients(void) {
                 continue;
 
             if (c->bstate.btype == BLOCKED_LAZYFREE) {
-                addReply(c, shared.ok); /* No reason lazy-free to fail */
+                /* SFLUSH: reply with empty array, FLUSH*: reply with OK */
+                if (c->cmd && c->cmd->proc == sflushCommand)
+                    addReplyArrayLen(c, 0);
+                else
+                    addReply(c, shared.ok);
                 updateStatsOnUnblock(c, 0, 0, 0);
                 c->flags &= ~CLIENT_PENDING_COMMAND;
                 unblockClient(c, 1);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-af3e530d45` → 本草稿移入 `cases/defect/auto-redis-af3e530d45/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
