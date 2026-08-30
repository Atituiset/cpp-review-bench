// AUTO-DRAFT from redis/redis PR #14974
targs[1] = RedisModule_CreateStringFromString(NULL, argv[1]);

    if (pthread_create(&tid, NULL, HelloACL_ThreadMain, targs) != 0) {
        RedisModule_AbortBlock(bc);
        return RedisModule_ReplyWithError(ctx, "-ERR Can't start thread");
    }
