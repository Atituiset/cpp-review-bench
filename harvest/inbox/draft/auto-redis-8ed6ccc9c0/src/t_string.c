// AUTO-DRAFT from redis/redis PR #15626
if (unit == UNIT_SECONDS) *milliseconds *= 1000;
  // <<< BUG ANCHOR
    if (relative_ttl) {
        *milliseconds += commandTimeSnapshot();
    }

    if (*milliseconds <= 0) {
        /* Overflow detected. */
        addReplyErrorExpireTime(c);
        return C_ERR;
    }
