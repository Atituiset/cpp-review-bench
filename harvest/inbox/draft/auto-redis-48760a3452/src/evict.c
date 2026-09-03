// AUTO-DRAFT from redis/redis PR #14934
/* The migrate client is like a replica, we also push DELs into it when
     * evicting keys belonging to the migrating slot, so we don't count its
     * output buffer to avoid eviction loop. */
    overhead += asmGetMigrateOutputBufferSize();  // <<< BUG ANCHOR

    if (server.aof_state != AOF_OFF) {
        overhead += sdsAllocSize(server.aof_buf);
