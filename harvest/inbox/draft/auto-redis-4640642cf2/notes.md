# auto-redis-4640642cf2

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15605 (https://github.com/redis/redis/pull/15605)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 653；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -648,17 +648,19 @@ dictEntry *kvstoreIteratorNext(kvstoreIterator *kvs_it) {
     return de;
 }
 
-/* This method traverses through kvstore dictionaries and triggers a resize.
- * It first tries to shrink if needed, and if it isn't, it tries to expand. */
-void kvstoreTryResizeDicts(kvstore *kvs, int limit) {
+/* This method traverses through kvstore dictionaries and triggers a resize,
+ * unless skip_cb indicates otherwise. It first tries to shrink if needed, and
+ * if it doesn't try to shrink, it tries to expand. */
+void kvstoreTryResizeDicts(kvstore *kvs, int limit, kvstoreResizeShouldSkipDictIndex *skip_cb) {
     if (limit > kvs->num_dicts)
         limit = kvs->num_dicts;
 
     for (int i = 0; i < limit; i++) {
         int didx = kvs->resize_cursor;
         dict *d = kvstoreGetDict(kvs, didx);
-        if (d && dictShrinkIfNeeded(d) == DICT_ERR) {
-            dictExpandIfNeeded(d);
+        if (d && (!skip_cb || !skip_cb(didx))) {
+            if (dictShrinkIfNeeded(d) == DICT_ERR)
+                dictExpandIfNeeded(d);
         }
         kvs->resize_cursor = (didx + 1) % kvs->num_dicts;
     }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-4640642cf2` → 本草稿移入 `cases/defect/auto-redis-4640642cf2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
