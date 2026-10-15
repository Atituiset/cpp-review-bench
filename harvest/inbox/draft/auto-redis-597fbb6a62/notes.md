# auto-redis-597fbb6a62

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15545 (https://github.com/redis/redis/pull/15545)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 625；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -619,10 +619,10 @@ int checkSignedBitfieldOverflow(int64_t value, int64_t incr, uint64_t bits, int
 
     /* Note that maxincr and minincr could overflow, but we use the values
      * only after checking 'value' range, so when we use it no overflow
-     * happens. 'uint64_t' cast is there just to prevent undefined behavior on
+     * happens. 'uint64_t' casts are there just to prevent undefined behavior on
      * overflow */
     int64_t maxincr = (uint64_t)max-value;
-    int64_t minincr = min-value;
+    int64_t minincr = (uint64_t)min-value;
 
     if (value > max || (bits != 64 && incr > maxincr) || (value >= 0 && incr > 0 && incr > maxincr))
     {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-597fbb6a62` → 本草稿移入 `cases/defect/auto-redis-597fbb6a62/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
