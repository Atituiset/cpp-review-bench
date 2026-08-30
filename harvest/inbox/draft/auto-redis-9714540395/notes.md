# auto-redis-9714540395

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15071 (https://github.com/redis/redis/pull/15071)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 220；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -213,12 +213,14 @@ sds sdsdup(const sds s) {
 /* Free an sds string. No operation is performed if 's' is NULL. */
 void sdsfree(sds s) {
     if (s == NULL) return;
-    s_free((char*)s-sdsHdrSize(s[-1]));
-}
-
-void sdsfreeusable(sds s, size_t *usable) {
-    if (s == NULL) return;
-    s_free_usable((char*)s-sdsHdrSize(s[-1]), usable);
+    if (sdsType(s) == SDS_TYPE_5) {
+        /* TYPE_5 has no alloc field so sdsAllocSize() returns the requested
+         * size which may not match the actual allocation, so not suitable for
+         * s_free_with_size(). */
+        s_free(sdsAllocPtr(s));
+    } else {
+        s_free_with_size(sdsAllocPtr(s), sdsAllocSize(s));
+    }
 }
 
 /* Generic version of sdsfree. */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-9714540395` → 本草稿移入 `cases/defect/auto-redis-9714540395/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
