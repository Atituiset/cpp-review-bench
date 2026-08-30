# auto-redis-071866b6f1

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15462 (https://github.com/redis/redis/pull/15462)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1087；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1084,7 +1084,13 @@ static void dictSetNext(dictEntry *de, dictEntry *next) {
 /* Returns the memory usage in bytes of the dict, excluding the size of the keys
  * and values. */
 size_t dictMemUsage(const dict *d) {
-    return dictSize(d) * sizeof(dictEntry) +
+    /* Account for the actual per-entry structure size: no_value=1 dicts (sets,
+     * the sorted-set element index, hashes) allocate a dictEntryNoValue (or
+     * store the key inline in the bucket), not a full dictEntry. Mirrors what
+     * kvstoreMemUsage() already does via dictEntryMemUsage(). This is a strict
+     * over-estimate still (inline-stored keys allocate nothing), but no longer
+     * charges the value slot that a no_value dict never has. */
+    return dictSize(d) * dictEntryMemUsage(d->type->no_value) +
         dictBuckets(d) * sizeof(dictEntry*);
 }
 
@@ -1993,6 +1999,15 @@ dictType BenchmarkDictType = {
     NULL
 };
 
+/* Same as BenchmarkDictType, but a no_value=1 (set-style) dict -- used to verify
+ * that dictMemUsage() sizes entries as dictEntryNoValue, not dictEntry. */
+static dictType BenchmarkDictTypeNoValue = {
+    .hashFunction = hashCallback,
+    .keyCompare = compareCallback,
+    .keyDestructor = freeCallback,
+    .no_value = 1,
+};
+
 #define start_benchmark() start = timeInMilliseconds()
 #define end_benchmark(msg) do { \
     elapsed = timeInMilliseconds()-start; \
@@ -2146,6 +2161,39 @@ int dictTest(int argc, char **argv, int flags) {
         dictEmpty(d, NULL);
         dictSetResizeEnabled(DICT_RESIZE_ENABLE);
     }
+
+    TEST("dictMemUsage sizes no_value entries by dictEntryNoValue (not dictEntry)") {
+        /* Regression: MEMORY USAGE used to overcount no_value=1 dicts (sets,
+         * the sorted-set element index, hashes) by charging sizeof(dictEntry)
+         * per entry instead of sizeof(dictEntryNoValue).
+         *
+         * A dictEntry is {next, key, value-union}; a dictEntryNoValue is
+         * {next, key}. Dropping the value makes a no_value entry smaller by
+         * exactly the size of the value union, which is 8 bytes (it holds a
+         * uint64_t/double) on both 64-bit (dictEntry 24 -> dictEntryNoValue 16)
+         * and 32-bit (16 -> 8). dictMemUsage() must reflect that 8 B/entry. */
+        const size_t value_union_bytes = 8;
+        assert(sizeof(dictEntry) - sizeof(dictEntryNoValue) == value_union_bytes);
+
+        long n = 1000;
+        dict *dn = dictCreate(&BenchmarkDictType);          /* no_value = 0 */
+        dict *dv = dictCreate(&BenchmarkDictTypeNoValue);   /* no_value = 1 */
+        for (long i = 0; i < n; i++) {
+            assert(dictAdd(dn, stringFromLongLong(i), (void *)i) == DICT_OK);
+            assert(dictAdd(dv, stringFromLongLong(i), NULL) == DICT_OK);
+        }
+
+        /* Identical keys and resize policy => identical bucket geometry, so the
+         * two dicts' reported memory differs only by the value union that each
+         * of the n no_value entries drops: n * 8 bytes. */
+        assert(dictSize(dn) == (unsigned long)n && dictSize(dv) == (unsigned long)n);
+        assert(dictBuckets(dn) == dictBuckets(dv));
+        assert(dictMemUsage(dn) - dictMemUsage(dv) == (size_t)n * value_union_bytes);
+
+        dictRelease(dn);
+        dictRelease(dv);
+    }
+
     srand(12345);
     start_benchmark();
     for (j = 0; j < count; j++) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-071866b6f1` → 本草稿移入 `cases/defect/auto-redis-071866b6f1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
