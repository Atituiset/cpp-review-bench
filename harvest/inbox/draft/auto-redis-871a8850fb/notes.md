# auto-redis-871a8850fb

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14950 (https://github.com/redis/redis/pull/14950)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 101；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -65,21 +65,21 @@
  *
  * (ASCII art adapted from https://brandur.org/rate-limiting). */
 
-/* GCRA key max_burst requests_per_period period [NUM_REQUESTS count]
+/* GCRA key max_burst tokens_per_period period [TOKENS count]
  *
  * key: Key related to specific rate limiting case
- * max_burst: Maximum requests allowed as burst (in addition to sustained rate)
- * requests_per_period: Number of requests allowed per period
+ * max_burst: Maximum tokens allowed as burst (in addition to sustained rate)
+ * tokens_per_period: Number of tokens allowed per period
  * period: Period in seconds for calculating sustained rate
- * num_requests: Optional, cost of this request (default: 1)
+ * tokens: Optional, cost of this request (default: 1)
  */
 void gcraCommand(client *c) {
     robj *key = c->argv[1];
 
     /* GCRA parameters */
     long max_burst;
-    long requests_per_period;
-    long num_requests = 1;
+    long tokens_per_period;
+    long num_tokens = 1;
     double period;
 
     /* Variables used in the reply */
@@ -98,7 +98,7 @@ void gcraCommand(client *c) {
     }
     if (likely(max_burst < LONG_MAX)) max_burst += 1;
 
-    if (getRangeLongFromObjectOrReply(c, c->argv[3], 1, LONG_MAX, &requests_per_period, NULL) != C_OK) {
+    if (getRangeLongFromObjectOrReply(c, c->argv[3], 1, LONG_MAX, &tokens_per_period, NULL) != C_OK) {
         return;
     }
 
@@ -111,15 +111,15 @@ void gcraCommand(client *c) {
     }
 
     if (c->argc >= 6) {
-        if (strcasecmp("NUM_REQUESTS", c->argv[5]->ptr)) {
+        if (strcasecmp("tokens", c->argv[5]->ptr)) {
             addReplyErrorObject(c, shared.syntaxerr);
             return;
         }
         if (c->argc == 6) {
-            addReplyError(c, "Missing NUM_REQUESTS value");
+            addReplyError(c, "Missing TOKENS value");
             return;
         }
-        if (getRangeLongFromObjectOrReply(c, c->argv[6], 1, LONG_MAX, &num_requests, NULL) != C_OK) {
+        if (getRangeLongFromObjectOrReply(c, c->argv[6], 1, LONG_MAX, &num_tokens, NULL) != C_OK) {
             return;
         }
     }
@@ -158,18 +158,18 @@ void gcraCommand(client *c) {
      * Even if emission_interval_us becomes less than 1us, we assume it's min
      * 1ms. The API is already in seconds granularity so it is expected the user
      * won't need a submicrosecond accuracy. */
-    long long emission_interval_us = (long long)(period_us / requests_per_period + 0.5);
+    long long emission_interval_us = (long long)(period_us / tokens_per_period + 0.5);
     if (unlikely(emission_interval_us == 0)) emission_interval_us = 1;
 
     /* overflow checks. In normal circumstances we shouldn't get these but the
      * user may have wrongfully specified very large values.
      * Note that all values are positive. */
-    if (emission_interval_us > LLONG_MAX / num_requests) {
-        addReplyError(c, "GCRA limiting uses microsecond accuracy. Combination of period, requests_per_period and num_requests would cause an overflow");
+    if (emission_interval_us > LLONG_MAX / num_tokens) {
+        addReplyError(c, "GCRA limiting uses microsecond accuracy. Combination of period, tokens_per_period and TOKENS would cause an overflow");
         return;
     }
     if (emission_interval_us > LLONG_MAX / max_burst) {
-        addReplyError(c, "GCRA limiting uses microsecond accuracy. Combination of period, requests_per_period and max_burst would cause an overflow");
+        addReplyError(c, "GCRA limiting uses microsecond accuracy. Combination of period, tokens_per_period and max_burst would cause an overflow");
         return;
     }
 
@@ -180,11 +180,11 @@ void gcraCommand(client *c) {
 
     /* If a request is allowed the next TaT is after an emission_interval_us time.
      * Hence for multiple requests we multiple by their number. */
-    long long increment_us = emission_interval_us * num_requests;
+    long long increment_us = emission_interval_us * num_tokens;
 
     long long base_us = (now > tat_us) 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-871a8850fb` → 本草稿移入 `cases/defect/auto-redis-871a8850fb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
