// AUTO-DRAFT from redis/redis PR #15338
#define CLUSTER_MANAGER_SLOTS               16384
#define CLUSTER_MANAGER_PORT_INCR           10000 /* same as CLUSTER_PORT_INCR */
#define CLUSTER_MANAGER_MIGRATE_TIMEOUT     60000
#define CLUSTER_MANAGER_MIGRATE_PIPELINE    10
#define CLUSTER_MANAGER_REBALANCE_THRESHOLD 2
  // <<< BUG ANCHOR
#define CLUSTER_MANAGER_INVALID_HOST_ARG \
    "[ERR] Invalid arguments: you need to pass either a valid " \
#define CLUSTER_MANAGER_CMD_FLAG_FIX_WITH_UNREACHABLE_MASTERS 1 << 10
#define CLUSTER_MANAGER_CMD_FLAG_MASTERS_ONLY   1 << 11
#define CLUSTER_MANAGER_CMD_FLAG_SLAVES_ONLY    1 << 12

#define CLUSTER_MANAGER_OPT_GETFRIENDS  1 << 0
#define CLUSTER_MANAGER_OPT_COLD        1 << 1
    return numCommands;
}

/* Gets the server version string by calling INFO SERVER.
 * Stores the result in config.server_version.
 * When not connected, or not possible, returns NULL. */
static sds cliGetServerVersion(void) {
    static const char *key = "\nredis_version:";
    redisReply *serverInfo = NULL;
    char *pos;

    if (config.server_version != NULL) {
        return config.server_version;
    }

    if (!context) return NULL;
    serverInfo = redisCommand(context, "INFO SERVER");
    if (serverInfo == NULL || serverInfo->type == REDIS_REPLY_ERROR) {
        freeReplyObject(serverInfo);
        return sdsempty();
        if (end) {
            sds version = sdsnewlen(pos, end - pos);
            freeReplyObject(serverInfo);
            config.server_version = version;
            return version;
        }
    }
    freeReplyObject(serverInfo);
    return NULL;
}

static void cliLegacyInitHelp(dict *groups) {
    sds serverVersion = cliGetServerVersion();
    
            config.cluster_manager_command.slots = atoi(argv[++i]);
        } else if (!strcmp(argv[i],"--cluster-timeout") && !lastarg) {
            config.cluster_manager_command.timeout = atoi(argv[++i]);
        } else if (!strcmp(argv[i],"--cluster-pipeline") && !lastarg) {
            config.cluster_manager_command.pipeline = atoi(argv[++i])
