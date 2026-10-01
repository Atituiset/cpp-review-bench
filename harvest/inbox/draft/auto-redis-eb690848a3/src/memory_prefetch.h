// AUTO-DRAFT from redis/redis PR #15133
#ifndef MEMORY_PREFETCH_H
#define MEMORY_PREFETCH_H

struct client;

void prefetchCommandsBatchInit(void);
int determinePrefetchCount(int len);
int addCommandToBatch(struct client *c);
void resetCommandsBatch(void);
void prefetchCommands(void);

#endif /* MEMORY_PREFETCH_H */
