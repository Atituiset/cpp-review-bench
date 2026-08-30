// AUTO-DRAFT from redis/redis PR #14690
/* Trying to create a sub-subcommand fails */
    RedisModule_Assert(RedisModule_CreateSubcommand(subcmd,"get",NULL,"",0,0,0) == REDISMODULE_ERR);

    return REDISMODULE_OK;
}
