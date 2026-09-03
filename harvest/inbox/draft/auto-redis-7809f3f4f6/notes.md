# auto-redis-7809f3f4f6

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14888 (https://github.com/redis/redis/pull/14888)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 126；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -123,7 +123,7 @@ static inline void lpAssertValidEntry(unsigned char* lp, size_t lpbytes, unsigne
 #define LISTPACK_MAX_SAFETY_SIZE (1<<30)
 int lpSafeToAdd(unsigned char* lp, size_t add) {
     size_t len = lp? lpGetTotalBytes(lp): 0;
-    if (len + add > LISTPACK_MAX_SAFETY_SIZE)
+    if (add > LISTPACK_MAX_SAFETY_SIZE || len > LISTPACK_MAX_SAFETY_SIZE - add)
         return 0;
     return 1;
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-7809f3f4f6` → 本草稿移入 `cases/defect/auto-redis-7809f3f4f6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
