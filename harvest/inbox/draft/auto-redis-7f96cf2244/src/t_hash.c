// AUTO-DRAFT from redis/redis PR #15628
return htemplates->by_id[chunk_idx];
}

/* Get lowest free id. Caller guarantees a gap exists. */
static size_t tmplIdGetLowestFree(void) {
    size_t chunk_idx = 0;
    while (chunk_idx < htemplates->by_id_cap && htemplates->by_id[chunk_idx] &&
           htemplates->by_id[chunk_idx]->used == TMPL_CHUNK_SIZE) {
        chunk_idx++;
    }
    tmplIdChunk *chunk = chunk_idx < htemplates->by_id_cap ? htemplates->by_id[chunk_idx] : NULL;
    size_t id = chunk_idx * TMPL_CHUNK_SIZE;
    while (chunk && chunk->slots[id % TMPL_CHUNK_SIZE] != NULL) id++;
static uint64_t tmplIdAllocate(hashTemplate *tmpl) {
    int no_gaps = dictSize(htemplates->by_fields) == htemplates->by_id_next;
    size_t id = no_gaps ? htemplates->by_id_next++ : tmplIdGetLowestFree();
    tmplIdChunk *chunk = tmplIdGetOrCreateChunk(id);
    chunk->slots[id % TMPL_CHUNK_SIZE] = tmpl;
    chunk->used++;
    size_t chunk_idx = id / TMPL_CHUNK_SIZE;
    tmplIdChunk *chunk = htemplates->by_id[chunk_idx];
    chunk->slots[id % TMPL_CHUNK_SIZE] = NULL;
    /* Free the chunk once it holds no live ids so the id space shrinks. */
    if (--chunk->used == 0) {
        zfree(chunk);
        htemplates->by_id_cap = 0;
        htemplates->by_id_chunks = 0;
        htemplates->by_id_next = 0;
    }
}


/* Defrag the template struct and re-point every reference
 * to it (by_id slot, by_fields key, by_fields_lp value).*/
hashTemplate *hashTemplateDefrag(hashTemplate *tmpl) {
    /* Field-name array and the strings it holds. */
    sds *newfields = activeDefragAlloc(tmpl->fields);
    if (newfields) tmpl->fields = newfields;
    for (unsigned long long i = 0; i < tmpl->field_count; i++) {
        sds newsds = activeDefragSds(tmpl->fields[i]);
        if (newsds) tmpl->fields[i] = newsds;
    }

    /* Find the entries referencing tmpl (by_fields key) and its blob
     * (by_fields_lp key+value) before any realloc frees the old pointers. */
    uint64_t bf_hash = dictGetHash(htemplates->by_fields, tmpl);
    dictEntry *
