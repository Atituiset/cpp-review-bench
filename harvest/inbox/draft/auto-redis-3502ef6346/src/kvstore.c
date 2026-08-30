// AUTO-DRAFT from redis/redis PR #14789
*/
int kvstoreExpand(kvstore *kvs, uint64_t newsize, int try_expand, kvstoreExpandShouldSkipDictIndex *skip_cb) {
    for (int i = 0; i < kvs->num_dicts; i++) {
        dict *d = kvstoreGetDict(kvs, i);  // <<< BUG ANCHOR
        if (!d || (skip_cb && skip_cb(i)))
            continue;
        int result = try_expand ? dictTryExpand(d, newsize) : dictExpand(d, newsize);
        if (try_expand && result == DICT_ERR)
            return 0;
