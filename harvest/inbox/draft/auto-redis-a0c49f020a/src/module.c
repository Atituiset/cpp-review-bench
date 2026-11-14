// AUTO-DRAFT from redis/redis PR #16042
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct RedisModuleCtx RedisModuleCtx;
/* …（同文件无关代码省略）… */
#define REDISMODULE_CTX_THREAD_SAFE (1<<4)
/* …（同文件无关代码省略）… */
#define REDISMODULE_CTX_COMMAND (1<<9) /* Context created to serve a command from call() or AOF (which calls cmd->proc directly) */
/* …（同文件无关代码省略）… */
typedef struct RedisModuleBlockedClient {
    client *client;  /* Pointer to the blocked client. or NULL if the client
                        was destroyed during the life of this object. */
    RedisModule *module;    /* Module blocking the client. */
    RedisModuleCmdFunc reply_callback; /* Reply callback on normal completion.*/
    RedisModuleAuthCallback auth_reply_cb; /* Reply callback on completing blocking
                                                    module authentication. */
    RedisModuleCmdFunc timeout_callback; /* Reply callback on timeout. */
    RedisModuleDisconnectFunc disconnect_callback; /* Called on disconnection.*/
    void (*free_privdata)(RedisModuleCtx*,void*);/* privdata cleanup callback.*/
    void *privdata;     /* Module private data that may be used by the reply
                           or timeout callback. It is set via the
                           RedisModule_UnblockClient() API. */
    client *thread_safe_ctx_client; /* Fake client to be used for thread safe
                                       context so that no lock is required. */
    client *reply_client;           /* Fake client used to accumulate replies
                                       in thread safe contexts. */
    int dbid;           /* Database number selected by the original client. */
    int blocked_on_keys;    /* If blocked via RM_BlockClientOnKeys(). */
    int unblocked;          /* Already on the moduleUnblocked list. */
    monotime background_timer; /* Timer tracking the start of background work */
    uint64_t background_duration; /* Current command background time duration.
                                     Used for measuring latency of blocking cmds */
    uint64_t background_duration_accounted; /* Slice of background_duration already
                                     used as leftover at GIL unlock / context free. */
    int blocked_on_keys_explicit_unblock; /* Set to 1 only in the case of an explicit RM_Unblock on
                                           * a client that is blocked on keys. In this case we will
                                           * call the timeout call back from within
                                           * moduleHandleBlockedClients which runs from the main thread */
} RedisModuleBlockedClient;
/* …（同文件无关代码省略）… */
static long moduleTakeUnaccountedBackgroundDuration(RedisModuleCtx *ctx) {
    if (server.execution_nesting) return 0;
    if (server.also_propagate.numops == 0) return 0;
    if (!ctx || !ctx->blocked_client) return 0;
    RedisModuleBlockedClient *bc = ctx->blocked_client;
    if (bc->background_duration <= bc->background_duration_accounted) return 0;
    long d = (long)(bc->background_duration - bc->background_duration_accounted);
    bc->background_duration_accounted = bc->background_duration;
    return d;
}
/* …（同文件无关代码省略）… */
/* Free the context after the user function was called. */
void moduleFreeContext(RedisModuleCtx *ctx) {
    /* See comment in moduleCreateContext */
    if (!(ctx->flags & (REDISMODULE_CTX_THREAD_SAFE|REDISMODULE_CTX_COMMAND))) {
        exitExecutionUnit();
        postExecutionUnitOperationsEx(moduleTakeUnaccountedBackgroundDuration(ctx));
    }
/* …（同文件无关代码省略）… */
     *    In the former, the counter has been already incremented from within
     *    call() and in the latter we don't care about execution_nesting
     * 2. If we are running in a thread (execution_nesting will be dealt with
     *    when locking/unlocking the GIL) */
    if (!(ctx_flags & (REDISMODULE_CTX_THREAD_SAFE|REDISMODULE_CTX_COMMAND))) {
        enterExecutionUnit(1, 0);
    }
}
