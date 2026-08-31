// AUTO-DRAFT from redis/redis PR #15441
}
        }

        /* The default behavior is to save the RDB file before loading
         * it back. */
        if (save) {
        serverLog(LL_NOTICE,"DB reloaded by DEBUG RELOAD");
        addReply(c,shared.ok);
    } else if (!strcasecmp(c->argv[1]->ptr,"loadaof")) {
        if (server.aof_state != AOF_OFF) flushAppendOnlyFile(1);
        emptyData(-1,EMPTYDB_NO_FLAGS,NULL);
        protectClient(c);
