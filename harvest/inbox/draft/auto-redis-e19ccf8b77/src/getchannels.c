// AUTO-DRAFT from redis/redis PR #16042
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

int getChannels_subscribe(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if ((argc - 1) % 3 != 0) {
        RedisModule_WrongArity(ctx);
        return REDISMODULE_OK;
    }
    char *err = NULL;
    
    /* getchannels.command [[subscribe|unsubscribe|publish] [pattern|literal] <channel> ...]
     * This command marks the given channel is accessed based on the
     * provided modifiers. */
    for (int i = 1; i < argc; i += 3) {
        const char *operation = RedisModule_StringPtrLen(argv[i], NULL);
        const char *type = RedisModule_StringPtrLen(argv[i+1], NULL);
        int flags = 0;

        if (!strcasecmp(operation, "subscribe")) {
            flags |= REDISMODULE_CMD_CHANNEL_SUBSCRIBE;
        } else if (!strcasecmp(operation, "unsubscribe")) {
            flags |= REDISMODULE_CMD_CHANNEL_UNSUBSCRIBE;
        } else if (!strcasecmp(operation, "publish")) {
            flags |= REDISMODULE_CMD_CHANNEL_PUBLISH;
        } else {
            err = "Invalid channel operation";
            break;
        }

        if (!strcasecmp(type, "literal")) {
            /* No op */
        } else if (!strcasecmp(type, "pattern")) {
            flags |= REDISMODULE_CMD_CHANNEL_PATTERN;
        } else {
            err = "Invalid channel type";
            break;
        }
        if (RedisModule_IsChannelsPositionRequest(ctx)) {
            RedisModule_ChannelAtPosWithFlags(ctx, i+2, flags);
        }
    }

    if (!RedisModule_IsChannelsPositionRequest(ctx)) {
        if (err) {
            RedisModule_ReplyWithError(ctx, err);
        } else {
            /* Normal implementation would go here, but for tests just return okay */
            RedisModule_ReplyWithSimpleString(ctx, "OK");
        }
    }

    return REDISMODULE_OK;
}
/* …（同文件无关代码省略）… */
    if (RedisModule_CreateCommand(ctx, "getchannels.command", getChannels_subscribe, "getchannels-api", 0, 0, 0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
