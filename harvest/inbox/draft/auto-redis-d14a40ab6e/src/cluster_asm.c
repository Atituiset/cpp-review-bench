// AUTO-DRAFT from redis/redis PR #16303
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct asmTask {
    sds id;                                 /* Task ID */
    int operation;                          /* Either ASM_IMPORT or ASM_MIGRATE */
    slotRangeArray *slots;                  /* List of slot ranges for this migration task */
    int state;                              /* Current state of the task */
    int dest_state;                         /* Destination node's main state (approximate) */
    char source[CLUSTER_NAMELEN];           /* Source node name */
    char dest[CLUSTER_NAMELEN];             /* Destination node name */
    connection *main_channel_conn;          /* Main channel connection */
    connection *rdb_channel_conn;           /* RDB channel connection */
    int rdb_channel_state;                  /* State of the RDB channel */
    unsigned long long dest_offset;         /* Destination offset */
    unsigned long long source_offset;       /* Source offset */
    int phase;                              /* Snapshot, or incremental */
    unsigned long long total_keys;          /* Total keys to migrate */
    unsigned long long migrated_keys;       /* Already migrated keys */
    int cross_slot_during_propagating;      /* If cross-slot commands are encountered during propagating */
    int stream_eof_during_streaming;        /* If STREAM-EOF is received during streaming buffer */
    replDataBuf sync_buffer;                /* Buffer for the stream */
    client *main_channel_client;            /* Client for the main channel on the source side */
    client *rdb_channel_client;             /* Client for the RDB channel on the source side */
    long long retry_count;                  /* Number of retries for this task */
    int retry_postponed;                    /* Logged that the retry waits for good replicas */
    mstime_t create_time;                   /* Task creation time */
    mstime_t start_time;                    /* Task start time */
    mstime_t end_time;                      /* Task end time */
    mstime_t paused_time;                   /* The time when the slot writes were paused */
    mstime_t dest_slots_snapshot_time;      /* The time when the destination starts applying the slot snapshot */
    mstime_t dest_accum_applied_time;       /* The time when the destination finishes applying the accumulated buffer */
    sds error;                              /* Error message for this task */
    redisOpArray *pre_snapshot_module_cmds; /* Module commands to be propagated at the beginning of slot migration */
    redisOpArray *post_stream_module_cmds;  /* Module commands to be propagated at the end of slot migration, just before STREAM-EOF */
} asmTask;
/* …（同文件无关代码省略）… */

/* Restart a failed import task. */
static void asmImportRetry(asmTask *task) {
    /* If min-replicas-to-write is not satisfied, this node rejects the
     * commands of the import stream and the task fails again. Do not
     * retry until there are enough good replicas, log it once. */
    if (!checkGoodReplicasStatus()) {
        if (!task->retry_postponed)
            serverLog(LL_NOTICE, "Import task %s is not retried, not enough good replicas", task->id);
        task->retry_postponed = 1;
        return;
    }
