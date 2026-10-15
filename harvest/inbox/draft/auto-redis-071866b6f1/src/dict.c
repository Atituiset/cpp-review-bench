// AUTO-DRAFT from redis/redis PR #15462
/* Returns the memory usage in bytes of the dict, excluding the size of the keys
 * and values. */
size_t dictMemUsage(const dict *d) {
    return dictSize(d) * sizeof(dictEntry) +  // <<< BUG ANCHOR
        dictBuckets(d) * sizeof(dictEntry*);
}

    NULL
};

#define start_benchmark() start = timeInMilliseconds()
#define end_benchmark(msg) do { \
    elapsed = timeInMilliseconds()-start; \
        dictEmpty(d, NULL);
        dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    }
    srand(12345);
    start_benchmark();
    for (j = 0; j < count; j++) {
