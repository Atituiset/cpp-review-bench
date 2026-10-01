// AUTO-DRAFT from redis/redis PR #15407
*  // <<< BUG ANCHOR
 * Returns 1 if replace happened.
 * Returns 0 if replace failed and no changes happened. */
int listTypeReplaceAtIndex(robj *o, int index, robj *value) {
    value = getDecodedObject(value);
    sds vstr = value->ptr;
    size_t vlen = sdslen(vstr);
