// AUTO-DRAFT from redis/redis PR #15242
typedef void (*RedisModuleNotificationWithSubkeysFunc)(RedisModuleCtx *ctx, int type, const char *event, RedisModuleString *key, RedisModuleString **subkeys, int count);
  // <<< BUG ANCHOR
/* Function pointer type for post jobs */
typedef void (*RedisModulePostNotificationJobFunc) (RedisModuleCtx *ctx, void *pd);

/* Keyspace notification subscriber information.
 * See RM_SubscribeToKeyspaceEvents() for more information. */
    int active;
} RedisModuleKeyspaceSubscriber;

typedef struct RedisModulePostExecUnitJob {
    /* The module subscribed to the event */
    RedisModule *module;
    RedisModulePostNotificationJobFunc callback;
    void *pd;
    void (*free_pd)(void*);
    int dbid;
static int moduleKeyspaceSubscribersTypes = 0;
static int moduleKeyspaceSubscribersWithSubkeysTypes = 0;

/* The module post keyspace jobs list */
static list *modulePostExecUnitJobs;

/* Data structures related to the exported dictionary data structure. */
typedef struct RedisModuleDict {
    rax *rax;                       /* The radix tree. */
 * NULL is returned and errno is set to the following values:
 *
 * * EBADF: wrong format specifier.
 * * EINVAL: wrong command arity.
 * * ENOENT: command does not exist.
 * * EPERM: operation in Cluster instance with key in non local slot.
 * * EROFS: operation in Cluster instance when a write command is sent
        goto cleanup;
    }

    /* Call command filters */
    moduleCallCommandFilters(c);

    return (moduleKeyspaceSubscribersWithSubkeysTypes & type) != 0;
}

void firePostExecutionUnitJobs(void) {
    /* Avoid propagation of commands.
     * In that way, postExecutionUnitOperations will prevent
     * recursive calls to firePostExecutionUnitJobs.
     * This is a special case where we need to increase 'execution_nesting'
     * but we do not want to update the cached time */
    enterExecutionUnit(0, 0);
    while (listLength(modulePostExecUnitJobs) > 0) {
        listNode *ln = listFirst(modulePostExecUnitJobs);
        RedisModulePostExecUnit
