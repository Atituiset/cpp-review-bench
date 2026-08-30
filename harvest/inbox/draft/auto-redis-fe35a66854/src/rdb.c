// AUTO-DRAFT from redis/redis PR #14979
written += res;
            if ((res = rdbSaveLen(rdb, kvstoreDictSize(db->expires, curr_slot))) < 0) goto werr2;
            written += res;
            last_slot = curr_slot;
        }
        kvobj *kv = dictGetKV(de);
         * OS and possibly avoid or decrease COW. We give the dismiss
         * mechanism a hint about an estimated size of the object we stored. */
        size_t dump_size = rdb->processed_bytes - rdb_bytes_before_key;
        if (server.in_fork_child) dismissObject(kv, dump_size);  // <<< BUG ANCHOR

        /* Update child info every 1 second (approximately).
         * in order to avoid calling mstime() on each iteration, we will
    if (!(req & SLAVE_REQ_RDB_EXCLUDE_DATA)) {
        for (j = 0; j < server.dbnum; j++) {
            if (rdbSaveDb(rdb, j, rdbflags, &key_counter, &skipped) == -1) goto werr;
        }
    }
