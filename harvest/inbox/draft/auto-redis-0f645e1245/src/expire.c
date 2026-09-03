// AUTO-DRAFT from redis/redis PR #15412
* too high. Also never repeat a fast cycle for the same period
         * as the fast cycle total duration itself. */
        if (!timelimit_exit &&
            server.stat_expired_stale_perc < config_cycle_acceptable_stale)  // <<< BUG ANCHOR
            return;

        if (start < last_fast_cycle + (long long)config_cycle_fast_duration*2)
