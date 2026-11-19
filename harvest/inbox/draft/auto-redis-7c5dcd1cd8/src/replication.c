// AUTO-DRAFT from redis/redis PR #16207
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
    if (compressed_stream)
        serverAssert(server.master->compression_state);

    blockingOperationStarts();
    while ((n = listFirst(buf->blocks))) {
        replDataBufBlock *o = listNodeValue(n);
        listUnlinkNode(buf->blocks, n);
        zfree(n);

        size_t processed = 0;
        while (processed < o->used) {
            /* Consumed bytes from the current block in the current iteration. */
            size_t consumed = 0;

            if (compressed_stream) {
                c->querybuf = sdsMakeRoomFor(c->querybuf, PROTO_IOBUF_LEN);
/* …（同文件无关代码省略）… */
                                                              c->querybuf + qblen,
                                                              avail,
                                                              &consumed);
                serverAssert(decompressed >= 0);
                sdsIncrLen(c->querybuf, decompressed);
                c->read_reploff += (long long) decompressed;
                c->io_read_reploff += (long long int) decompressed;

                atomicIncr(server.stat_net_repl_input_decompressed_bytes, decompressed);
            } else {
/* …（同文件无关代码省略）… */
                c->read_reploff += (long long int) bytes;
                c->io_read_reploff += (long long int) bytes;
                consumed = bytes;
            }
            c->lastinteraction = server.unixtime;

/* …（同文件无关代码省略）… */

            /* Check if we should yield back to the event loop */
            if (server.loading_process_events_interval_bytes &&
                ((ctx->applied_offset + consumed) / server.loading_process_events_interval_bytes >
                  ctx->applied_offset / server.loading_process_events_interval_bytes))
            {
                ctx->yield_callback(ctx);
                processEventsWhileBlocked();
            }
            ctx->applied_offset += consumed;

            /* Check if we should continue processing */
            if (!ctx->should_continue(ctx)) {
