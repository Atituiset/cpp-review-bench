// AUTO-DRAFT from redis/redis PR #15628
* RESTORE find the template with one O(1) blob lookup.*/
    mstime_t fields_lp_last_used; /* Last time fields_lp was used, for cron idle reclaim. */
    unsigned int fits_in_listpack;  /* 1 if fields fit in listpack (DUMP serializes them as LP blob) */
} hashTemplate;

/* Global registry for hash templates. */
    size_t by_id_cap;           /* How many chunk pointers by_id can hold. */
    size_t by_id_chunks;        /* How many chunks are currently allocated. */
    size_t by_id_next;          /* The next id that has never been used. */
    size_t total_key_refs;      /* Sum of key_refcount across all templates. */
    size_t fields_lp_cache_bytes; /* Total lpBytes() of cached fields listpack blobs. */
    size_t total_mem_size;      /* Sum of every live template's mem_size, plus any
hashTemplate *hashTemplateGetOrCreate(sds *fields, unsigned long long field_count);
hashTemplate *hashTemplateGetByFieldsLp(unsigned char *fields_lp);
hashTemplate *hashTemplateGetById(uint64_t id);
hashTemplate *hashTemplateDefrag(hashTemplate *tmpl);
int hashTemplateDefragByIdChunk(unsigned long chunk_idx);
hashTemplate *hashTypeGetTemplate(robj *o);
void hashTemplateIncrKeyRef(hashTemplate *tmpl);
