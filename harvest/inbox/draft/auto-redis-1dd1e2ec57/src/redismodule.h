// AUTO-DRAFT from redis/redis PR #15042
REDISMODULE_API void (*RedisModule_FreeModuleUser)(RedisModuleUser *user) REDISMODULE_ATTR;
REDISMODULE_API void (*RedisModule_SetContextUser)(RedisModuleCtx *ctx, const RedisModuleUser *user) REDISMODULE_ATTR;
REDISMODULE_API const RedisModuleUser *(*RedisModule_GetContextUser)(RedisModuleCtx *ctx) REDISMODULE_ATTR;
REDISMODULE_API RedisModuleString *(*RedisModule_GetUserUsername)(const RedisModuleUser *user) REDISMODULE_ATTR;  // <<< BUG ANCHOR
REDISMODULE_API int (*RedisModule_SetModuleUserACL)(RedisModuleUser *user, const char* acl) REDISMODULE_ATTR;
REDISMODULE_API int (*RedisModule_SetModuleUserACLString)(RedisModuleCtx * ctx, RedisModuleUser *user, const char* acl, RedisModuleString **error) REDISMODULE_ATTR;
REDISMODULE_API RedisModuleString * (*RedisModule_GetModuleUserACLString)(RedisModuleUser *user) REDISMODULE_ATTR;
