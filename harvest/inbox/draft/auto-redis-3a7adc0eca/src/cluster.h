// AUTO-DRAFT from redis/redis PR #14953
int clusterIsMySlot(int slot);
int clusterCanAccessKeysInSlot(int slot);
struct slotRangeArray *clusterGetLocalSlotRanges(void);

/* functions with shared implementations */
clusterNode *getNodeByQuery(client *c, struct redisCommand *cmd, robj **argv, int argc, int *hashslot,
