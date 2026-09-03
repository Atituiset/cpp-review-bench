# auto-redis-2100426eb4

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15282 (https://github.com/redis/redis/pull/15282)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 171；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -155,6 +155,30 @@ typedef struct streamPropInfo {
     robj *groupname;
 } streamPropInfo;
 
+/* Parameters controlling how streamReplyWithRange() fetches and emits entries.
+ * Bundled into a struct so callers can use designated initializers and avoid a
+ * long, error-prone positional argument list. Fields left unset are zero, which
+ * matches "no group", "no propagation", "no limit", etc.; note that
+ * 'min_idle_time' must be set to -1 to disable the idle-time filter. */
+typedef struct streamReplyRangeArgs {
+    streamID *start;            /* Inclusive range start (NULL for open start). */
+    streamID *end;              /* Inclusive range end (NULL for open end). */
+    size_t count;               /* Max entries to emit (0 means unlimited). */
+    int rev;                    /* Iterate in reverse order if non-zero. */
+    long long min_idle_time;    /* Only serve PEL entries idle for this long;
+                                   -1 disables idle-time filtering. */
+    streamCG *group;            /* Consumer group, or NULL. */
+    streamConsumer *consumer;   /* Consumer within the group, or NULL. */
+    int flags;                  /* STREAM_RWR_* flags. */
+    streamPropInfo *spi;        /* Propagation info, or NULL for no propagation. */
+    unsigned long *propCount;   /* Out: number of propagated commands, or NULL. */
+    long long maxsize;          /* Byte budget for the reply (0 means unlimited).
+                                   Already includes the output-bytes baseline, so
+                                   MAXSIZE is checked against the absolute
+                                   c->net_output_bytes_curr_cmd. */
+    size_t emitted_before;      /* Entries already emitted before this call. */
+} streamReplyRangeArgs;
+
 /* Prototypes of exported APIs. */
 struct client;
 
@@ -168,7 +192,7 @@ struct client;
 stream *streamNew(void);
 void freeStream(stream *s);
 unsigned long streamLength(const robj *subject);
-size_t streamReplyWithRange(client *c, stream *s, streamID *start, streamID *end, size_t count, int rev, long long min_idle_time, streamCG *group, streamConsumer *consumer, int flags, streamPropInfo *spi, unsigned long *propCount);
+size_t streamReplyWithRange(client *c, stream *s, streamReplyRangeArgs *args);
 void streamIteratorStart(streamIterator *si, stream *s, streamID *start, streamID *end, int rev);
 int streamIteratorGetID(streamIterator *si, streamID *id, int64_t *numfields);
 void streamIteratorGetField(streamIterator *si, unsigned char **fieldptr, unsigned char **valueptr, int64_t *fieldlen, int64_t *valuelen);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-2100426eb4` → 本草稿移入 `cases/defect/auto-redis-2100426eb4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
