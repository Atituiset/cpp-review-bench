# auto-redis-4253abb9a6

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15662 (https://github.com/redis/redis/pull/15662)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 797；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -794,7 +794,7 @@ uint32_t random_level(void) {
     static const int threshold = HNSW_P * RAND_MAX;
     uint32_t level = 0;
 
-    while (rand() < threshold && level < HNSW_MAX_LEVEL)
+    while (rand() < threshold && level < HNSW_MAX_LEVEL - 1)
         level += 1;
     return level;
 }
@@ -903,6 +903,13 @@ uint32_t hnsw_quants_bytes(HNSW *index) {
  * after the node creation (see later for the serialization API that
  * handles this and more). */
 hnswNode *hnsw_node_new(HNSW *index, uint64_t id, const float *vector, const int8_t *qvector, float qrange, uint32_t level, int normalize) {
+    /* Defense in depth: every node level must stay within the architectural
+     * cap. Untrusted input (e.g. a tampered serialized stream) is already
+     * rejected by the caller before reaching this point, so a violation here
+     * can only be an internal bug: fail fast instead of over-allocating the
+     * node and poisoning index->max_level. */
+    assert(level < HNSW_MAX_LEVEL);
+
     hnswNode *node = hmalloc(sizeof(hnswNode)+(sizeof(hnswNodeLayer)*(level+1)));
     if (!node) return NULL;
 
@@ -2593,6 +2600,12 @@ hnswNode *hnsw_insert_serialized(HNSW *index, void *vector, uint64_t *params, ui
     uint32_t version = (params[1] & 0xff000000) >> 24;  // Format version.
 
     if (version > HNSW_SERIALIZATION_VERSION) return NULL;
+
+    /* A serialized stream coming from an untrusted source (e.g. a tampered
+     * RDB file) could encode an out of range level: reject it here, otherwise
+     * we would over-allocate the node layers and poison index->max_level with
+     * an illegal value. */
+    if (level >= HNSW_MAX_LEVEL) return NULL;
     int has_worst_link_info = version > 0;
 
     /* Keep track of maximum ID seen while loading. */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-4253abb9a6` → 本草稿移入 `cases/defect/auto-redis-4253abb9a6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
