// AUTO-DRAFT from redis/redis PR #15432
*
 * * RedisModuleEvent_ForkChild
 *
 *     Called when a fork child (AOFRW, RDBSAVE, module fork...) is born or dies,
 *     and around the fork itself so a multi-threaded module can bring its
 *     background threads to a safe point before `fork()`. This matters because a
 *     thread holding a lock (for example the allocator lock) at `fork()` time
 *     would deadlock the child the first time it tries to take that lock.
 *     `REDISMODULE_SUBEVENT_FORK_CHILD_PRE` is fired synchronously on the main
 *     thread right before `fork()` (returning from the handler tells Redis the
 *     module is ready); the module then resumes on
 *     `REDISMODULE_SUBEVENT_FORK_CHILD_BORN` if the fork happened or on
 *     `REDISMODULE_SUBEVENT_FORK_CHILD_CANCELLED` if it did not.
 *     The following sub events are available:
 *
 *     * `REDISMODULE_SUBEVENT_FORK_CHILD_BORN`
 *     * `REDISMODULE_SUBEVENT_FORK_CHILD_DIED`
 *     * `REDISMODULE_SUBEVENT_FORK_CHILD_PRE`
 *     * `REDISMODULE_SUBEVENT_FORK_CHILD_CANCELLED`
 *
 * * RedisModuleEvent_EventLoop
 *
