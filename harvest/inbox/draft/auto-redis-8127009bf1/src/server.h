// AUTO-DRAFT from redis/redis PR #15680
void replicationStartPendingFork(void);
void replicationHandleMasterDisconnection(void);
void replicationCacheMaster(client *c);
void resizeReplicationBacklog(void);
void replicationSetMaster(char *ip, int port);
void replicationUnsetMaster(void);
