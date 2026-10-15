# auto-redis-9e53a1895d

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15710 (https://github.com/redis/redis/pull/15710)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 36；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -32,8 +32,14 @@ typedef enum monotonic_clock_type {
 /* Call once at startup to initialize the monotonic clock.  Though this only
  * needs to be called once, it may be called additional times without impact.
  * Returns a printable string indicating the type of clock initialized.
- * (The returned string is static and doesn't need to be freed.)  */
-const char *monotonicInit(void);
+ * (The returned string is static and doesn't need to be freed.)
+ *
+ * 'logger' is a printf-alike used to report notes from the clock detection
+ * and calibration fallback paths (e.g. an unconfirmed TSC rate); the server
+ * passes a serverLog() wrapper.  Pass NULL to discard those notes -- nothing
+ * is written to stderr, as this file is linked into every binary and some
+ * callers treat any child stderr output as failure.  */
+const char *monotonicInit(void (*logger)(const char *fmt, ...));
 
 /* Return a string indicating the type of monotonic clock being used. */
 const char *monotonicInfoString(void);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-9e53a1895d` → 本草稿移入 `cases/defect/auto-redis-9e53a1895d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
