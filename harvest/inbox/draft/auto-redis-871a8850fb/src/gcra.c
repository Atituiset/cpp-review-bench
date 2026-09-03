// AUTO-DRAFT from redis/redis PR #14950
*
 * (ASCII art adapted from https://brandur.org/rate-limiting). */
  // <<< BUG ANCHOR
/* GCRA key max_burst requests_per_period period [NUM_REQUESTS count]
 *
 * key: Key related to specific rate limiting case
 * max_burst: Maximum requests allowed as burst (in addition to sustained rate)
 * requests_per_period: Number of requests allowed per period
 * period: Period in seconds for calculating sustained rate
 * num_requests: Optional, cost of this request (default: 1)
 */
void gcraCommand(client *c) {
    robj *key = c->argv[1];

    /* GCRA parameters */
    long max_burst;
    long requests_per_period;
    long num_requests = 1;
    double period;

    /* Variables used in the reply */
    }
    if (likely(max_burst < LONG_MAX)) max_burst += 1;

    if (getRangeLongFromObjectOrReply(c, c->argv[3], 1, LONG_MAX, &requests_per_period, NULL) != C_OK) {
        return;
    }

    }

    if (c->argc >= 6) {
        if (strcasecmp("NUM_REQUESTS", c->argv[5]->ptr)) {
            addReplyErrorObject(c, shared.syntaxerr);
            return;
        }
        if (c->argc == 6) {
            addReplyError(c, "Missing NUM_REQUESTS value");
            return;
        }
        if (getRangeLongFromObjectOrReply(c, c->argv[6], 1, LONG_MAX, &num_requests, NULL) != C_OK) {
            return;
        }
    }
     * Even if emission_interval_us becomes less than 1us, we assume it's min
     * 1ms. The API is already in seconds granularity so it is expected the user
     * won't need a submicrosecond accuracy. */
    long long emission_interval_us = (long long)(period_us / requests_per_period + 0.5);
    if (unlikely(emission_interval_us == 0)) emission_interval_us = 1;

    /* overflow checks. In normal circumstances we shouldn't get these but the
     * user may have wrongfully specified very large values.
     * Note that all values are positive. */
    if (emission_interval_us > LLONG_MAX / num_requests) {
        addReplyError(c, "GCRA limiting uses microsecond accuracy. Combination of period,
