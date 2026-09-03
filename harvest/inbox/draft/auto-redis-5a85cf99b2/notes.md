# auto-redis-5a85cf99b2

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15133 (https://github.com/redis/redis/pull/15133)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 235；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -19,18 +19,50 @@
 #include "server.h"
 #include "dict.h"
 
-typedef enum { HT_IDX_FIRST = 0, HT_IDX_SECOND = 1, HT_IDX_INVALID = -1 } HashTableIndex;
-
-typedef enum {
-    PREFETCH_BUCKET,     /* Initial state, determines which hash table to use and prefetch the table's bucket */
-    PREFETCH_ENTRY,      /* prefetch entries associated with the given key's hash */
-    PREFETCH_KVOBJ,      /* prefetch the kv object of the entry found in the previous step */
-    PREFETCH_VALDATA,    /* prefetch the value data of the kv object found in the previous step */
-    PREFETCH_DONE        /* Indicates that prefetching for this key is complete */
-} PrefetchState;
+/* --------------------------------------------------------------------------
+ * Dict prefetching state machine
+ * -------------------------------------------------------------------------- */
 
+typedef enum { HT_IDX_FIRST = 0, HT_IDX_SECOND = 1, HT_IDX_INVALID = -1 } dictHtIdx;
 
-/************************************ State machine diagram for the prefetch operation. ********************************
+typedef enum {
+    PREFETCH_BUCKET,        /* Initial state, determines which hash table to use and prefetch the table's bucket */
+    PREFETCH_ENTRY,         /* prefetch entries associated with the given key's hash */
+    PREFETCH_ENTRY_KEY,     /* dictType-driven prefetch of the entry's key payload (for keyCompare) */
+    PREFETCH_ENTRY_VALUE,   /* compare keys; on match, dictType-driven prefetch of the value payload */
+    PREFETCH_DONE           /* Indicates that prefetching for this key is complete */
+} dictPrefetchState;
+
+/* Per-key state of an in-flight, software-pipelined dictFind, advanced one
+ * stage at a time by dictPrefetcher (see below). The non-state fields mirror
+ * the locals that a synchronous dictFind would otherwise carry across one
+ * bucket walk. */
+typedef struct dictPrefetchLookup {
+    dictPrefetchState state;  /* Current FSM stage of this lookup */
+    dictHtIdx ht_idx;         /* Index of the current hash table (0 or 1 for rehashing) */
+    uint64_t bucket_idx;      /* Index of the bucket in the current hash table */
+    uint64_t key_hash;        /* Hash value of the key being looked up */
+    dictEntry *current_entry; /* Pointer to the current entry being processed */
+} dictPrefetchLookup;
+
+/* dictPrefetcher drives a batch of dictPrefetchLookup objects through the
+ * prefetch FSM, yielding to the next in-flight lookup each time a prefetch
+ * is issued — so one lookup's memory stall overlaps another's work. The
+ * state machine itself is fully dict-pure: any key/value payload prefetching
+ * is delegated to the dictType->prefetchEntryKey / prefetchEntryValue
+ * callbacks of each key's dict. The same prefetcher is used by both the
+ * cross-command batch path and the intra-command dictPrefetchKeys() API. */
+typedef struct dictPrefetcher {
+    size_t cur_idx;              /* Cursor; advances on each prefetch issue */
+    size_t nkeys;                /* Total key lookups in this batch */
+    size_t remaining;            /* Number of in-flight key lookups (not yet PREFETCH_DONE) */
+    void **keys;                 /* Array of key pointers (sds) */
+    dict **dicts;                /* Per-key dictionary pointers */
+    dictPrefetchLookup *lookups; /* Per-key lookup state, capacity == max_keys */
+    size_t max_keys;             /* Capacity of lookups[] */
+} dictPrefetcher;
+
+/******************************** State machine diagram for the dict prefetch operation. ******************************
                                                            │
                                                          start
                                                            │
@@ -44,33 +76,254 @@ typedef enum {
                                     ┌────────────►└────────┬────────┘              │
                                     |                 Entry│found                  │
                                    
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-5a85cf99b2` → 本草稿移入 `cases/defect/auto-redis-5a85cf99b2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
