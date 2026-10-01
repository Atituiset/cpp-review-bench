// AUTO-DRAFT from redis/redis PR #15203
client *old_client = server.current_client;
        server.current_client = c;
        enterExecutionUnit(1, 0);
        processCommandAndResetClient(c);  // <<< BUG ANCHOR
        if (!(c->flags & CLIENT_BLOCKED)) {
            if (c->flags & CLIENT_MODULE) {
                moduleCallCommandUnblockedHandler(c);
