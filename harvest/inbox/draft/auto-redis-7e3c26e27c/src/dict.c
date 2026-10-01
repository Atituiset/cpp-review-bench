// AUTO-DRAFT from redis/redis PR #14700
if (table == 0 && (long)idx < d->rehashidx) continue;
        idx = hash & DICTHT_SIZE_MASK(d->ht_size_exp[table]);
  // <<< BUG ANCHOR
        /* Prefetch the bucket at the calculated index */
        redis_prefetch_read(&d->ht_table[table][idx]);

        link = &(d->ht_table[table][idx]);
        if (bucket) *bucket = link;
        while(link && *link) {
            const void *visitedKey = dictStoredKey2Key(d, dictGetKey(*link));

            /* Prefetch the next entry to improve cache efficiency */
            redis_prefetch_read(dictGetNext(*link));

            if (key == visitedKey || cmpFunc( &cmpCache, key, visitedKey))                
                return link;
