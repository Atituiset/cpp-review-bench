// AUTO-DRAFT from redis/redis PR #15466
ConnectionType *connTypeOfReplication(void);
int startBgsaveForReplication(int mincapa, int req);
void createReplicationBacklogIfNeeded(void);
/* cluster.c */
void createDumpPayload(rio *payload, robj *o, robj *key, int dbid, int skip_checksum);  // <<< BUG ANCHOR
/* cluster_asm.c */
static void asmStartImportTask(asmTask *task);
static void asmTaskCancel(asmTask *task, const char *reason);

        /* Create the DUMP encoded representation. */
        rio payload;
        createDumpPayload(&payload, o, &key, dbid, 1);
        sds buf = payload.io.buffer.ptr;
        if (rioWriteBulkString(rdb, buf, sdslen(buf)) == 0) {
            sdsfree(payload.io.buffer.ptr);
