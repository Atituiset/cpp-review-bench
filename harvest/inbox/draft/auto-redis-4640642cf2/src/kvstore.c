// AUTO-DRAFT from redis/redis PR #15605
return de;
}
  // <<< BUG ANCHOR
/* This method traverses through kvstore dictionaries and triggers a resize.
 * It first tries to shrink if needed, and if it isn't, it tries to expand. */
void kvstoreTryResizeDicts(kvstore *kvs, int limit) {
    if (limit > kvs->num_dicts)
        limit = kvs->num_dicts;

    for (int i = 0; i < limit; i++) {
        int didx = kvs->resize_cursor;
        dict *d = kvstoreGetDict(kvs, didx);
        if (d && dictShrinkIfNeeded(d) == DICT_ERR) {
            dictExpandIfNeeded(d);
        }
        kvs->resize_cursor = (didx + 1) % kvs->num_dicts;
    }
