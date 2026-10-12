// AUTO-DRAFT from redis/redis PR #15545
/* Note that maxincr and minincr could overflow, but we use the values
     * only after checking 'value' range, so when we use it no overflow
     * happens. 'uint64_t' cast is there just to prevent undefined behavior on
     * overflow */
    int64_t maxincr = (uint64_t)max-value;
    int64_t minincr = min-value;  // <<< BUG ANCHOR

    if (value > max || (bits != 64 && incr > maxincr) || (value >= 0 && incr > 0 && incr > maxincr))
    {
