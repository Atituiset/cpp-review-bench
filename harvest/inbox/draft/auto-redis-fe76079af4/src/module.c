// AUTO-DRAFT from redis/redis PR #15995
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct RedisModuleCtx RedisModuleCtx;
/* …（同文件无关代码省略）… */
struct RedisModuleKey {
    RedisModuleCtx *ctx;
    redisDb *db;
    robj *key;      /* Key name object. */
    kvobj *kv;      /* Key-Value object, or NULL if the key was not found. */
    void *iter;     /* Iterator. */
    int mode;       /* Opening mode. */

    union {
        struct {
            /* List, use only if value->type == OBJ_LIST */
            listTypeEntry entry;   /* Current entry in iteration. */
            long index;            /* Current 0-based index in iteration. */
        } list;
        struct {
            /* Zset iterator, use only if value->type == OBJ_ZSET */
            uint32_t type;         /* REDISMODULE_ZSET_RANGE_* */
            zrangespec rs;         /* Score range. */
            zlexrangespec lrs;     /* Lex range. */
            uint32_t start;        /* Start pos for positional ranges. */
            uint32_t end;          /* End pos for positional ranges. */
            void *current;         /* Zset iterator current node. */
            int er;                /* Zset iterator end reached flag
                                       (true if end was reached). */
        } zset;
        struct {
            /* Stream, use only if value->type == OBJ_STREAM */
            streamID currentid;    /* Current entry while iterating. */
            int64_t numfieldsleft; /* Fields left to fetch for current entry. */
            int signalready;       /* Flag that signalKeyAsReady() is needed. */
        } stream;
    } u;
};
/* …（同文件无关代码省略）… */
int RM_DeleteKey(RedisModuleKey *key) {
    if (!(key->mode & REDISMODULE_WRITE)) return REDISMODULE_ERR;
    if (key->kv) {
        dbDelete(key->db,key->key);
        key->kv = NULL;
    }
    return REDISMODULE_OK;
}
/* …（同文件无关代码省略）… */
 * writing or there is an active iterator, REDISMODULE_ERR is returned. */
int RM_ModuleTypeSetValue(RedisModuleKey *key, moduleType *mt, void *value) {
    if (!(key->mode & REDISMODULE_WRITE) || key->iter) return REDISMODULE_ERR;
    RM_DeleteKey(key);
    robj *o = createModuleObject(mt,value);
    setKey(key->ctx->client,key->db,key->key, &o,SETKEY_NO_SIGNAL);
    key->kv = o;
    return REDISMODULE_OK;
}
