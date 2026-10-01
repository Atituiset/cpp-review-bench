# auto-redis-f452dc5838

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14817 (https://github.com/redis/redis/pull/14817)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 13（原始 PR diff 行 219；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -200,6 +200,13 @@ void enableTracking(client *c, uint64_t redirect_to, uint64_t options, robj **pr
  * that should receive an invalidation message with certain groups of keys
  * are modified. */
 void trackingRememberKeys(client *tracking, client *executing) {
+    /* Shard channels are treated as special keys for client
+     * library to rely on `COMMAND` command to discover the node
+     * to connect to. These channels don't need to be tracked. */
+    if (executing->cmd->flags & CMD_PUBSUB) {
+        return;
+    }
+
     /* Return if we are in optin/out mode and the right CACHING command
      * was/wasn't given in order to modify the default behavior. */
     uint64_t optin = tracking->flags & CLIENT_TRACKING_OPTIN;
@@ -213,12 +220,6 @@ void trackingRememberKeys(client *tracking, client *executing) {
         getKeysFreeResult(&result);
         return;
     }
-    /* Shard channels are treated as special keys for client
-     * library to rely on `COMMAND` command to discover the node
-     * to connect to. These channels doesn't need to be tracked. */
-    if (executing->cmd->flags & CMD_PUBSUB) {
-        return;
-    }
 
     keyReference *keys = result.keys;
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-f452dc5838` → 本草稿移入 `cases/defect/auto-redis-f452dc5838/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
