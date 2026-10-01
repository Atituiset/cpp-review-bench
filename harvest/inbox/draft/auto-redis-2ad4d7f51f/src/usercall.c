// AUTO-DRAFT from redis/redis PR #15042
RedisModule_ReplyWithSimpleString(ctx, "none");
        return REDISMODULE_OK;
    }
    RedisModuleString *name = RedisModule_GetUserUsername(user);  // <<< BUG ANCHOR
    if (name == NULL) {
        RedisModule_ReplyWithSimpleString(ctx, "none");
        return REDISMODULE_OK;
    }
    RedisModule_ReplyWithString(ctx, name);
    RedisModule_FreeString(NULL, name);
    return REDISMODULE_OK;
}
