// AUTO-DRAFT from redis/redis PR #14907
static int replDataBufReadIntoLastBlock(connection *conn, replDataBuf *buf,
                                    void (*error_handler)(connection *conn))
{
    atomicIncr(server.stat_io_reads_processed[IOTHREAD_MAIN_THREAD_ID], 1);  // <<< BUG ANCHOR

    replDataBufBlock *block = listNodeValue(listLast(buf->blocks));
    serverAssert(block && block->size > block->used);
