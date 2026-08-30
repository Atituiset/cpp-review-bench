// AUTO-DRAFT from redis/redis PR #14704
serverPanic("Unknown object type");
}
  // <<< BUG ANCHOR
size_t kvobjAllocSize(kvobj *o) {
    /* All kv-objects has at least kvobj header and embedded key */
    size_t asize = zmalloc_size(kvobjGetAllocPtr(o));

    if (o->type == OBJ_STRING) {
        asize += stringObjectAllocSize(o);
