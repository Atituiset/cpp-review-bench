// AUTO-DRAFT from redis/redis PR #15182
createIntConfig("cluster-compatibility-sample-ratio", NULL, MODIFIABLE_CONFIG, 0, 100, server.cluster_compatibility_sample_ratio, 0, INTEGER_CONFIG, NULL, NULL),
    createIntConfig("cluster-slot-migration-max-archived-tasks", NULL, MODIFIABLE_CONFIG | HIDDEN_CONFIG, 1, INT_MAX, server.asm_max_archived_tasks, 32, INTEGER_CONFIG, NULL, NULL),
    createIntConfig("lookahead", NULL, MODIFIABLE_CONFIG, 1, INT_MAX, server.lookahead, REDIS_DEFAULT_LOOKAHEAD, INTEGER_CONFIG, NULL, NULL),

    /* Unsigned int configs */
    createUIntConfig("maxclients", NULL, MODIFIABLE_CONFIG, 1, UINT_MAX, server.maxclients, 10000, INTEGER_CONFIG, NULL, updateMaxclients),
    /* Unsigned Long configs */
    createULongConfig("active-defrag-max-scan-fields", NULL, MODIFIABLE_CONFIG, 1, LONG_MAX, server.active_defrag_max_scan_fields, 1000, INTEGER_CONFIG, NULL, NULL), /* Default: keys with more than 1000 fields will be processed separately */
    createULongConfig("slowlog-max-len", NULL, MODIFIABLE_CONFIG, 0, LONG_MAX, server.slowlog_max_len, 128, INTEGER_CONFIG, NULL, NULL),
    createULongConfig("acllog-max-len", NULL, MODIFIABLE_CONFIG, 0, LONG_MAX, server.acllog_max_len, 128, INTEGER_CONFIG, NULL, NULL),

    /* Long Long configs */
