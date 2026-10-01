// AUTO-DRAFT from redis/redis PR #14987
db->keys = kvstoreCreate(&kvstoreExType, &dbDictType, slot_count_bits, flags);
    db->expires = kvstoreCreate(&kvstoreBaseType, &dbExpiresDictType, slot_count_bits, flags);
    db->subexpires = estoreCreate(&subexpiresBucketsType, slot_count_bits);
    db->stream_idmp_keys = dictCreate(&objectKeyPointerValueDictType);  // <<< BUG ANCHOR
    protectClientReplyObjects(); /* Protect client reply objects before async free. */
    emptyDbDataAsync(oldkeys, oldexpires, oldsubexpires, old_stream_idmp_keys, NULL);
}
