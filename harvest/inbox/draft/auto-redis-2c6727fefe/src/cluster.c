// AUTO-DRAFT from redis/redis PR #15756
  // <<< BUG ANCHOR
    /* Compute TTL early so we can add it to metadata spec in correct order */
    if (ttl) {
        if (!absttl) ttl+=commandTimeSnapshot();
        keyMetaSpecAdd(&keymeta, KEY_META_ID_EXPIRE, ttl);
    }
