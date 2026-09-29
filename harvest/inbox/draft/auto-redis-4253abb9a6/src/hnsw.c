// AUTO-DRAFT from redis/redis PR #15662
static const int threshold = HNSW_P * RAND_MAX;
    uint32_t level = 0;
  // <<< BUG ANCHOR
    while (rand() < threshold && level < HNSW_MAX_LEVEL)
        level += 1;
    return level;
}
 * after the node creation (see later for the serialization API that
 * handles this and more). */
hnswNode *hnsw_node_new(HNSW *index, uint64_t id, const float *vector, const int8_t *qvector, float qrange, uint32_t level, int normalize) {
    hnswNode *node = hmalloc(sizeof(hnswNode)+(sizeof(hnswNodeLayer)*(level+1)));
    if (!node) return NULL;

    uint32_t version = (params[1] & 0xff000000) >> 24;  // Format version.

    if (version > HNSW_SERIALIZATION_VERSION) return NULL;
    int has_worst_link_info = version > 0;

    /* Keep track of maximum ID seen while loading. */
