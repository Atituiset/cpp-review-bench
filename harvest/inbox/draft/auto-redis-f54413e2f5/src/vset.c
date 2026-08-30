// AUTO-DRAFT from redis/redis PR #15230
/* Add the 0.33 remaining part, but upper layers have less links. */
    size += (sizeof(hnswNode*) * other_levels_links * vset->hnsw->node_count)/3;

    /* Associated string value and attributres.
     * Use Redis Module API to get string size, and guess that all the
     * elements have similar size as the first few. */
    size_t items_scanned = 0, items_size = 0;
    if (items_scanned)
        size += items_size / items_scanned * vset->hnsw->node_count;

    /* Add memory usage due to attributres. */
    if (attribs_scanned == 0) {
        /* We were not lucky enough to find a single attribute in the
         * first few items? Let's use a fixed arbitrary value. */
