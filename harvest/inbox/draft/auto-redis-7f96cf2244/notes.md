# auto-redis-7f96cf2244

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15628 (https://github.com/redis/redis/pull/15628)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 489；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -400,13 +400,20 @@ static tmplIdChunk *tmplIdGetOrCreateChunk(size_t id) {
     return htemplates->by_id[chunk_idx];
 }
 
-/* Get lowest free id. Caller guarantees a gap exists. */
+/* Get lowest free id. Caller guarantees a gap exists. Chunks below
+ * by_id_free_chunk_hint are full, the scan starts there. */
 static size_t tmplIdGetLowestFree(void) {
-    size_t chunk_idx = 0;
+    size_t chunk_idx = htemplates->by_id_free_chunk_hint;
+#ifdef DEBUG_ASSERTIONS
+    /* Chunks below the hint must be full. */
+    for (size_t i = 0; i < chunk_idx; i++)
+        serverAssert(htemplates->by_id[i] && htemplates->by_id[i]->used == TMPL_CHUNK_SIZE);
+#endif
     while (chunk_idx < htemplates->by_id_cap && htemplates->by_id[chunk_idx] &&
            htemplates->by_id[chunk_idx]->used == TMPL_CHUNK_SIZE) {
         chunk_idx++;
     }
+    htemplates->by_id_free_chunk_hint = chunk_idx;
     tmplIdChunk *chunk = chunk_idx < htemplates->by_id_cap ? htemplates->by_id[chunk_idx] : NULL;
     size_t id = chunk_idx * TMPL_CHUNK_SIZE;
     while (chunk && chunk->slots[id % TMPL_CHUNK_SIZE] != NULL) id++;
@@ -418,6 +425,8 @@ static size_t tmplIdGetLowestFree(void) {
 static uint64_t tmplIdAllocate(hashTemplate *tmpl) {
     int no_gaps = dictSize(htemplates->by_fields) == htemplates->by_id_next;
     size_t id = no_gaps ? htemplates->by_id_next++ : tmplIdGetLowestFree();
+    /* Every lower id is in use, the hint can move up. */
+    if (no_gaps) htemplates->by_id_free_chunk_hint = id / TMPL_CHUNK_SIZE;
     tmplIdChunk *chunk = tmplIdGetOrCreateChunk(id);
     chunk->slots[id % TMPL_CHUNK_SIZE] = tmpl;
     chunk->used++;
@@ -429,6 +438,8 @@ static void tmplIdRecycle(uint64_t id) {
     size_t chunk_idx = id / TMPL_CHUNK_SIZE;
     tmplIdChunk *chunk = htemplates->by_id[chunk_idx];
     chunk->slots[id % TMPL_CHUNK_SIZE] = NULL;
+    if (htemplates->by_id_free_chunk_hint > chunk_idx)
+        htemplates->by_id_free_chunk_hint = chunk_idx;
     /* Free the chunk once it holds no live ids so the id space shrinks. */
     if (--chunk->used == 0) {
         zfree(chunk);
@@ -446,6 +457,7 @@ static void tmplIdRecycle(uint64_t id) {
         htemplates->by_id_cap = 0;
         htemplates->by_id_chunks = 0;
         htemplates->by_id_next = 0;
+        htemplates->by_id_free_chunk_hint = 0;
     }
 }
 
@@ -459,41 +471,57 @@ hashTemplate *hashTemplateGetById(uint64_t id) {
 
 /* Defrag the template struct and re-point every reference
  * to it (by_id slot, by_fields key, by_fields_lp value).*/
-hashTemplate *hashTemplateDefrag(hashTemplate *tmpl) {
-    /* Field-name array and the strings it holds. */
+void hashTemplateDefrag(hashTemplate *tmpl, dictEntry *bf, monotime endtime) {
+    /* Field-name array. */
     sds *newfields = activeDefragAlloc(tmpl->fields);
     if (newfields) tmpl->fields = newfields;
-    for (unsigned long long i = 0; i < tmpl->field_count; i++) {
+
+    /* Field names, checking the clock every 128 so a wide template cannot
+     * blow the time budget. On timeout, defrag_field keeps the position, the
+     * leftover is picked up when the template is scanned again. */
+    unsigned long long i = tmpl->defrag_field;
+    long iterations = 0;
+    while (i < tmpl->field_count) {
         sds newsds = activeDefragSds(tmpl->fields[i]);
         if (newsds) tmpl->fields[i] = newsds;
+        server.stat_active_defrag_scanned++;
+        i++;
+        if (++iterations > 128) {
+            if (getMonotonicUs() > endtime) break;
+            iterations = 0;
+        }
     }
-
-    /* Find the entries referencing tmpl (by_fields key) and its blob
-     * (by_fields_lp key+value) before any realloc frees the old pointers. */
-    uint64_t bf_hash = dictGetHash(htemplates->by_fields, tmpl);
-    dictEntry *bf = dictFindByHashAndPtr(htemplates->by_fields, tmpl, bf_hash);
-    dictEntry *lp = tmpl->fields_lp ? dictFind(htemplates->by_fields_lp, tmpl->fields_lp) : NULL;
+    tmpl->defrag_field = (i >= tmpl->field_count) ? 0 : i;
 
  
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-7f96cf2244` → 本草稿移入 `cases/defect/auto-redis-7f96cf2244/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
