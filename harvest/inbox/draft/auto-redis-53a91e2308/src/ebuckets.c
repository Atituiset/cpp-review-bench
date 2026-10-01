// AUTO-DRAFT from redis/redis PR #15256
/* Use min expire-time for the first segment in rax */
    unsigned char raxKey[EB_KEY_SIZE];
    bucketKey2RaxKey(bucketKey, raxKey);
    rax *rax = raxNewWithMetadata(sizeof(uint64_t), NULL);  // <<< BUG ANCHOR
    *ebRaxNumItems(rax) = EB_LIST_MAX_ITEMS;
    raxInsert(rax, raxKey, EB_KEY_SIZE, firstSegHdr, NULL);
    return rax;
        if (newSegHdr) {
            if (currentSegHdr == ri->data) {
                /* If it's the first segment, update the rax data pointer. */
                raxSetData(ri->node, ri->data=newSegHdr);
                firstSegHdr = newSegHdr;
            } else {
                /* For non-first segments, update the previous segment's next
