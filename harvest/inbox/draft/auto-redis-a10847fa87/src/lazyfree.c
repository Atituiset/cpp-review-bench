// AUTO-DRAFT from redis/redis PR #14608
}
}

/* Empty a Redis DB asynchronously. What the function does actually is to
 * create a new empty set of hash tables and scheduling the old ones for
 * lazy freeing. */
    db->keys = kvstoreCreate(&kvstoreExType, &dbDictType, slot_count_bits, flags);
    db->expires = kvstoreCreate(&kvstoreBaseType, &dbExpiresDictType, slot_count_bits, flags);
    db->subexpires = estoreCreate(&subexpiresBucketsType, slot_count_bits);
    emptyDbDataAsync(oldkeys, oldexpires, oldsubexpires);
}
