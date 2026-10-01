# auto-redis-011794279d

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15282 (https://github.com/redis/redis/pull/15282)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 40；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -37,7 +37,7 @@
 
 void streamFreeCGGeneric(void *cg, void *s);
 void streamFreeNACK(stream *s, streamNACK *na);
-size_t streamReplyWithRangeFromConsumerPEL(client *c, stream *s, streamID *start, streamID *end, size_t count, streamCG *group, streamConsumer *consumer);
+size_t streamReplyWithRangeFromConsumerPEL(client *c, stream *s, streamID *start, streamID *end, size_t count, streamCG *group, streamConsumer *consumer, long long maxsize, size_t emitted_before);
 int streamParseStrictIDOrReply(client *c, robj *o, streamID *id, uint64_t missing_seq, int *seq_given);
 int streamParseIDOrReply(client *c, robj *o, streamID *id, uint64_t missing_seq);
 
@@ -1964,6 +1964,16 @@ void streamPropagateConsumerCreation(client *c, robj *key, robj *groupname, sds
     decrRefCount(argv[4]);
 }
 
+/* Returns non-zero if the MAXSIZE byte budget has been reached. 'maxsize' is the
+ * budget (0 = no limit) and 'emitted' the entries emitted so far; the budget is
+ * only enforced after at least one entry, so a single oversized message can still
+ * exceed it. 'maxsize' already includes the output-bytes baseline (output bytes at
+ * serve-start), so earlier commands in the same MULTI/EXEC don't count; outside a
+ * transaction the baseline is 0. */
+static inline int streamReplyMaxsizeReached(client *c, long long maxsize, size_t emitted) {
+    return maxsize && emitted > 0 && c->net_output_bytes_curr_cmd >= (size_t)maxsize;
+}
+
 /* Send the stream items in the specified range to the client 'c'. The range
  * the client will receive is between start and end inclusive, if 'count' is
  * non zero, no more than 'count' elements are sent.
@@ -2005,6 +2015,15 @@ void streamPropagateConsumerCreation(client *c, robj *key, robj *groupname, sds
  * STREAM_RWR_CLAIMED: Return only claimable entries from the PEL. New entries
  *                     from the stream are not returned.
  *
+ * The 'maxsize' argument, when non-zero, is a byte budget for the whole command
+ * reply (XREAD/XREADGROUP MAXSIZE). Once the accumulated reply size
+ * (c->net_output_bytes_curr_cmd) reaches 'maxsize', this function stops emitting
+ * further entries. 'emitted_before' is the number of entries already emitted by
+ * previous streams in the same command; together with the entries emitted here
+ * it implements the "a single oversized message may exceed maxsize" exception:
+ * the budget is never enforced before at least one entry has been emitted across
+ * the whole reply.
+ *
  * The final argument 'spi' (stream propagation info pointer) is a structure
  * filled with information needed to propagate the command execution to AOF
  * and slaves, in the case a consumer group was passed: we need to generate
@@ -2026,7 +2045,19 @@ void streamPropagateConsumerCreation(client *c, robj *key, robj *groupname, sds
                                            boundaries, just the entries. */
 #define STREAM_RWR_HISTORY (1<<2)       /* Only serve consumer local PEL. */
 #define STREAM_RWR_CLAIMED (1<<3)       /* Only serve claimed entries from PEL. */
-size_t streamReplyWithRange(client *c, stream *s, streamID *start, streamID *end, size_t count, int rev, long long min_idle_time, streamCG *group, streamConsumer *consumer, int flags, streamPropInfo *spi, unsigned long *propCount) {
+size_t streamReplyWithRange(client *c, stream *s, streamReplyRangeArgs *args) {
+    streamID *start = args->start;
+    streamID *end = args->end;
+    size_t count = args->count;
+    int rev = args->rev;
+    long long min_idle_time = args->min_idle_time;
+    streamCG *group = args->group;
+    streamConsumer *consumer = args->consumer;
+    int flags = args->flags;
+    streamPropInfo *spi = args->spi;
+    unsigned long *propCount = args->propCount;
+    long long maxsize = args->maxsize;
+    size_t emitted_before = args->emitted_before;
     void *arraylen_ptr = NULL;
     size_t arraylen = 0;
     streamIterator si;
@@ -2072,6 +2103,9 @@ size_t streamReplyWithRange(client *c, stream *s, streamID
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-011794279d` → 本草稿移入 `cases/defect/auto-redis-011794279d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
