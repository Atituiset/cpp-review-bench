# auto-redis-dbdb9cc281

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15628 (https://github.com/redis/redis/pull/15628)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 4096；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3981,6 +3981,7 @@ typedef struct hashTemplate {
                           * RESTORE find the template with one O(1) blob lookup.*/
     mstime_t fields_lp_last_used; /* Last time fields_lp was used, for cron idle reclaim. */
     unsigned int fits_in_listpack;  /* 1 if fields fit in listpack (DUMP serializes them as LP blob) */
+    unsigned int defrag_field;      /* Defrag resume point into 'fields'. */
 } hashTemplate;
 
 /* Global registry for hash templates. */
@@ -3993,6 +3994,7 @@ typedef struct hashTemplateRegistry {
     size_t by_id_cap;           /* How many chunk pointers by_id can hold. */
     size_t by_id_chunks;        /* How many chunks are currently allocated. */
     size_t by_id_next;          /* The next id that has never been used. */
+    size_t by_id_free_chunk_hint; /* Lowest chunk index that may hold a free id. */
     size_t total_key_refs;      /* Sum of key_refcount across all templates. */
     size_t fields_lp_cache_bytes; /* Total lpBytes() of cached fields listpack blobs. */
     size_t total_mem_size;      /* Sum of every live template's mem_size, plus any
@@ -4093,7 +4095,7 @@ void hashTemplatesInit(void);
 hashTemplate *hashTemplateGetOrCreate(sds *fields, unsigned long long field_count);
 hashTemplate *hashTemplateGetByFieldsLp(unsigned char *fields_lp);
 hashTemplate *hashTemplateGetById(uint64_t id);
-hashTemplate *hashTemplateDefrag(hashTemplate *tmpl);
+void hashTemplateDefrag(hashTemplate *tmpl, dictEntry *bf, monotime endtime);
 int hashTemplateDefragByIdChunk(unsigned long chunk_idx);
 hashTemplate *hashTypeGetTemplate(robj *o);
 void hashTemplateIncrKeyRef(hashTemplate *tmpl);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-dbdb9cc281` → 本草稿移入 `cases/defect/auto-redis-dbdb9cc281/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
