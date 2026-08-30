// AUTO-DRAFT from redis/redis PR #14330
void dictTwoPhaseUnlinkFree(dict *d, dictEntryLink llink, int table_index);
void dictRelease(dict *d);
dictEntry * dictFind(dict *d, const void *key);
int dictShrinkIfNeeded(dict *d);
int dictExpandIfNeeded(dict *d);
void *dictGetKey(const dictEntry *de);
