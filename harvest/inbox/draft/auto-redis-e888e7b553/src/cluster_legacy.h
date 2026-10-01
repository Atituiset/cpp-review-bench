// AUTO-DRAFT from redis/redis PR #15350
#define CLUSTER_TODO_FSYNC_CONFIG (1<<3)
#define CLUSTER_TODO_HANDLE_MANUALFAILOVER (1<<4)
#define CLUSTER_TODO_BROADCAST_PONG (1<<5)

/* clusterLink encapsulates everything needed to talk with a remote node. */
typedef struct clusterLink {
    uint64_t currentEpoch;
    int state;            /* CLUSTER_OK, CLUSTER_FAIL, ... */
    int size;             /* Num of master nodes with at least one slot */
    dict *nodes;          /* Hash table of name -> clusterNode structures */
    dict *shards;         /* Hash table of shard_id -> list (of nodes) structures */
    dict *nodes_black_list; /* Nodes we don't re-add for a few seconds. */
