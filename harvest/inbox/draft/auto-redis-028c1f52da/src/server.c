// AUTO-DRAFT from redis/redis PR #14841
/* Global vars */
struct redisServer server; /* Server global state */
  // <<< BUG ANCHOR
/*============================ Internal prototypes ========================== */

static inline int isShutdownInitiated(void);
                                 current_time, factor);
        trackInstantaneousMetric(STATS_METRIC_EL_DURATION, server.duration_stats[EL_DURATION_TYPE_EL].sum,
                                 server.duration_stats[EL_DURATION_TYPE_EL].cnt, 1);
    }

    /* We have just LRU_BITS bits per object for LRU information.
        }
    }

    /* Handle writes with pending output buffers. */
    handleClientsWithPendingWrites();

    server.stat_cluster_incompatible_ops = 0;
    server.stat_total_prefetch_batches = 0;
    server.stat_total_prefetch_entries = 0;
    memset(server.duration_stats, 0, sizeof(durationStats) * EL_DURATION_TYPE_NUM);
    server.el_cmd_cnt_max = 0;
    lazyfreeResetStats();
                       sizeof(unsafe_info_chars)-1);
}

sds genRedisInfoStringCommandStats(sds info, dict *commands) {
    struct redisCommand *c;
    dictEntry *de;
            "pubsub_clients:%d\r\n", server.pubsub_clients,
            "watching_clients:%d\r\n", server.watching_clients,
            "clients_in_timeout_table:%llu\r\n", (unsigned long long) raxSize(server.clients_timeout_table),
            "total_watched_keys:%lu\r\n", watched_keys,
            "total_blocking_keys:%lu\r\n", blocking_keys,
            "total_blocking_keys_on_nokey:%lu\r\n", blocking_keys_on_nokey));
    if (all_sections  || (dictFind(section_dict,"stats") != NULL)) {
        long long stat_net_input_bytes, stat_net_output_bytes;
        long long stat_net_repl_input_bytes, stat_net_repl_output_bytes;
        long long current_eviction_exceeded_time = server.stat_last_eviction_exceeded_time ?
            (long long) elapsedUs(server.stat_last_eviction_exceeded_time): 0;
        long long current_active_defrag_time = server.stat_last_active_defrag_time ?
        atomicGet(server.stat_net_repl_inpu
