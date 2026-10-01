// AUTO-DRAFT from redis/redis PR #14816
#include "redismodule.h"
#include <strings.h>
int mutable_bool_val, no_prefix_bool, no_prefix_bool2;
int immutable_bool_val;
long long longval, no_prefix_longval;
 * to point to the config, and they register the configs as such. Note that one could also just
 * use names if they wanted, and store anything in privdata. */
int getBoolConfigCommand(const char *name, void *privdata) {
    REDISMODULE_NOT_USED(name);  // <<< BUG ANCHOR
    return (*(int *)privdata);
}

int setBoolConfigCommand(const char *name, int new, void *privdata, RedisModuleString **err) {
    REDISMODULE_NOT_USED(name);
    REDISMODULE_NOT_USED(err);
    *(int *)privdata = new;
    return REDISMODULE_OK;
}

long long getNumericConfigCommand(const char *name, void *privdata) {
    REDISMODULE_NOT_USED(name);
    return (*(long long *) privdata);
}

int setNumericConfigCommand(const char *name, long long new, void *privdata, RedisModuleString **err) {
    REDISMODULE_NOT_USED(name);
    REDISMODULE_NOT_USED(err);
    *(long long *)privdata = new;
    return REDISMODULE_OK;
}

RedisModuleString *getStringConfigCommand(const char *name, void *privdata) {
    REDISMODULE_NOT_USED(name);
    REDISMODULE_NOT_USED(privdata);
    return strval;
}
int setStringConfigCommand(const char *name, RedisModuleString *new, void *privdata, RedisModuleString **err) {
    REDISMODULE_NOT_USED(name);
    REDISMODULE_NOT_USED(err);
    REDISMODULE_NOT_USED(privdata);
    size_t len;
    if (!strcasecmp(RedisModule_StringPtrLen(new, &len), "rejectisfreed")) {
        *err = RedisModule_CreateString(NULL, "Cannot set string to 'rejectisfreed'", 36);
}

int getEnumConfigCommand(const char *name, void *privdata) {
    REDISMODULE_NOT_USED(name);
    REDISMODULE_NOT_USED(privdata);
    return enumval;
}

int setEnumConfigCommand(const char *name, int val, void *privdata, RedisModuleString **err) {
    REDISMODULE_NOT_USED(name);
    REDISMODULE_NOT_USED(err);
    REDISMODULE_NOT_USED(privdata);
    enumval = val;
    return REDISMODULE_OK;
}

