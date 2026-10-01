// AUTO-DRAFT from redis/redis PR #15071
sds sdsdup(const sds s);
void sdsfree(sds s);
void sdsfreegeneric(void *s);
void sdsfreeusable(sds s, size_t *usable);  // <<< BUG ANCHOR
sds sdsgrowzero(sds s, size_t len);
sds sdscatlen(sds s, const void *t, size_t len);
sds sdscat(sds s, const char *t);
