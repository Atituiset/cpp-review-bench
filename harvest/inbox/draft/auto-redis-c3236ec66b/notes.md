# auto-redis-c3236ec66b

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15499 (https://github.com/redis/redis/pull/15499)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 67；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -45,6 +45,9 @@ struct compressionState {
                                    * or NULL if not pending. */
     int handle_pending;  /* When set, clientConnRead only drains already-read
                           * compressed data without touching the socket. */
+    size_t alloc_size;  /* Total allocated size of this struct plus the input/output
+                         * buffers, captured at allocation time so memory accounting
+                         * doesn't have to query the allocator on every call. */
 };
 
 /* --- zstd --- */
@@ -64,17 +67,20 @@ static int zstdInitCompress(compressionState *st, int level) {
 
     /* temp buf storing compressed data */
     size_t outSize = ZSTD_CStreamOutSize();
-    st->output.data = zmalloc(outSize);
+    size_t usable = 0;
+    st->output.data = zmalloc_usable(outSize, &usable);
     st->output.size = outSize;
     st->output.written = 0;
     st->output.consumed = 0;
+    st->alloc_size += usable;
 
     /* temp buf storing uncompressed data */
     size_t inSize = ZSTD_CStreamInSize();
-    st->input.data = zmalloc(inSize);
+    st->input.data = zmalloc_usable(inSize, &usable);
     st->input.size = inSize;
     st->input.written = 0;
     st->input.consumed = 0;
+    st->alloc_size += usable;
 
     st->write_flush_pending = 0;
 
@@ -90,17 +96,20 @@ static int zstdInitDecompress(compressionState *st) {
 
     /* temp buf storing compressed data */
     size_t inSize = ZSTD_DStreamInSize();
-    st->input.data = zmalloc(inSize);
+    size_t usable = 0;
+    st->input.data = zmalloc_usable(inSize, &usable);
     st->input.size = inSize;
     st->input.written = 0;
     st->input.consumed = 0;
+    st->alloc_size += usable;
 
     /* temp buf storing decompressed data */
     size_t outSize = ZSTD_DStreamOutSize();
-    st->output.data = zmalloc(outSize);
+    st->output.data = zmalloc_usable(outSize, &usable);
     st->output.size = outSize;
     st->output.written = 0;
     st->output.consumed = 0;
+    st->alloc_size += usable;
 
     st->read_flush_pending = 0;
 
@@ -233,7 +242,9 @@ static const compressionType zstdType = {
 
 /* Create compression state for the client */
 int compressionStateCreate(client *c) {
-    compressionState *st = zcalloc(sizeof(compressionState));
+    size_t usable = 0;
+    compressionState *st = zcalloc_usable(sizeof(compressionState), &usable);
+    st->alloc_size = usable;
     st->type = &zstdType;
     st->last_write = 0;
     st->write_flush_pending = 0;
@@ -553,6 +564,20 @@ int clientHasPendingCompressedData(client *c) {
            state->output.written > state->output.consumed;
 }
 
+/* Return the number of bytes used by the client's compression state, i.e the
+ * compressionState struct itself plus its input/output buffers. The size is
+ * captured at allocation time (see compressionStateCreate/zstdInitCompress/
+ * zstdInitDecompress) so this call is just a field read instead of querying
+ * the allocator. Note this does not include the zstd compression/decompression
+ * context (ctx.zstdCCtx/zstdDCtx), which is allocated by libzstd via libc
+ * malloc and therefore tracked neither here nor in used_memory. */
+size_t clientCompressionMemoryUsage(client *c) {
+    compressionState *st = c->compression_state;
+    if (!st) return 0;
+
+    return st->alloc_size;
+}
+
 /* Add the client to its event loop's pending decompression list so its buffered
  * compressed/decompressed data can be drained from beforeSleep even when no
  * socket read event fires. No-op if already present. */
@@ -819,6 +844,11 @@ int clientHasPendingCompressedData(client *c) {
     return 0;
 }
 
+size_t clientCompressionMemoryUsage(client *c) {
+    UNUSED(c);
+    return 0;
+}
+
 int clientConnWrite(client *c, const void *data, size_t len, int *nwritten) {
     int w = connWrite(c->conn, data, len);
     if (nwritten) *nwritten = (w > 0) ? w : 0;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-c3236ec66b` → 本草稿移入 `cases/defect/auto-redis-c3236ec66b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
