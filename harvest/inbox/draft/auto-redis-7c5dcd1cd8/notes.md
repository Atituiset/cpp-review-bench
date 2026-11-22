# auto-redis-7c5dcd1cd8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#16207](https://github.com/redis/redis/pull/16207) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16207 (https://github.com/redis/redis/pull/16207)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 4268；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16207 Fix replica-side bugs when compression is enabled :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4258,16 +4258,27 @@ int replDataBufStreamToDb(replDataBuf *buf, replDataBufToDbCtx *ctx) {
     if (compressed_stream)
         serverAssert(server.master->compression_state);
 
+    /* Bytes consumed from all blocks so far, used to pace yielding to the
+     * event loop. Unlike ctx->applied_offset it counts input (possibly
+     * compressed) bytes, and unlike `processed` it isn't reset per block. */
+    size_t total_consumed = 0;
+
     blockingOperationStarts();
     while ((n = listFirst(buf->blocks))) {
         replDataBufBlock *o = listNodeValue(n);
         listUnlinkNode(buf->blocks, n);
         zfree(n);
 
         size_t processed = 0;
-        while (processed < o->used) {
+        /* Copying all compressed bytes into the decompressor does not mean
+         * their commands have been applied. Drain its pending output before
+         * leaving the streaming state, even if this block is exhausted. */
+        while (processed < o->used ||
+               (compressed_stream && clientHasPendingCompressedData(c))) {
             /* Consumed bytes from the current block in the current iteration. */
             size_t consumed = 0;
+            /* Bytes passed to the command parser, after decompression (if decompression is needed). */
+            size_t applied = 0;
 
             if (compressed_stream) {
                 c->querybuf = sdsMakeRoomFor(c->querybuf, PROTO_IOBUF_LEN);
@@ -4279,10 +4290,14 @@ int replDataBufStreamToDb(replDataBuf *buf, replDataBufToDbCtx *ctx) {
                                                               c->querybuf + qblen,
                                                               avail,
                                                               &consumed);
-                serverAssert(decompressed >= 0);
+                if (decompressed < 0) {
+                    ret = C_ERR;
+                    break;
+                }
                 sdsIncrLen(c->querybuf, decompressed);
                 c->read_reploff += (long long) decompressed;
                 c->io_read_reploff += (long long int) decompressed;
+                applied = decompressed;
 
                 atomicIncr(server.stat_net_repl_input_decompressed_bytes, decompressed);
             } else {
@@ -4291,6 +4306,7 @@ int replDataBufStreamToDb(replDataBuf *buf, replDataBufToDbCtx *ctx) {
                 c->read_reploff += (long long int) bytes;
                 c->io_read_reploff += (long long int) bytes;
                 consumed = bytes;
+                applied = bytes;
             }
             c->lastinteraction = server.unixtime;
 
@@ -4320,13 +4336,14 @@ int replDataBufStreamToDb(replDataBuf *buf, replDataBufToDbCtx *ctx) {
 
             /* Check if we should yield back to the event loop */
             if (server.loading_process_events_interval_bytes &&
-                ((ctx->applied_offset + consumed) / server.loading_process_events_interval_bytes >
-                  ctx->applied_offset / server.loading_process_events_interval_bytes))
+                ((total_consumed + consumed) / server.loading_process_events_interval_bytes >
+                  total_consumed / server.loading_process_events_interval_bytes))
             {
                 ctx->yield_callback(ctx);
                 processEventsWhileBlocked();
             }
-            ctx->applied_offset += consumed;
+            total_consumed += consumed;
+            ctx->applied_offset += applied;
 
             /* Check if we should continue processing */
             if (!ctx->should_continue(ctx)) {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`atomicIncr`
- 外部函数：`blockingOperationStarts`
- 外部函数：`listFirst`
- 外部函数：`listNodeValue`
- 外部函数：`listUnlinkNode`
- 外部函数：`processEventsWhileBlocked`
- 外部函数：`sdsIncrLen`
- 外部函数：`sdsMakeRoomFor`
- 外部函数：`serverAssert`
- 外部函数：`should_continue`
- 外部函数：`yield_callback`
- 外部函数：`zfree`
- 大写宏：`PROTO_IOBUF_LEN`
- 外部类型：`Check`
- 外部类型：`Consumed`
- 外部类型：`size_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-redis-7c5dcd1cd8` → 本草稿移入 `cases/defect/auto-redis-7c5dcd1cd8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
