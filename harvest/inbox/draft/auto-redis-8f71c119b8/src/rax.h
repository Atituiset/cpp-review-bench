// AUTO-DRAFT from redis/redis PR #15252
void *privdata;         /* Optional private data for node callback. */
} raxIterator;

/* Exported API. */
rax *raxNew(void);
rax *raxNewWithMetadata(int metaSize, size_t *alloc_size);
int raxInsert(rax *rax, unsigned char *s, size_t len, void *data, void **old);
int raxTryInsert(rax *rax, unsigned char *s, size_t len, void *data, void **old);
int raxRemove(rax *rax, unsigned char *s, size_t len, void **old);
int raxFind(rax *rax, unsigned char *s, size_t len, void **value);
void raxFree(rax *rax);
void raxFreeWithCallback(rax *rax, void (*free_callback)(void*));
void raxFreeWithCbAndContext(rax *rax,
