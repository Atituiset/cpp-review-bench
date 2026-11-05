// AUTO-DRAFT from redis/redis PR #15972
/* Arm the deadline unless it is already counting down. */
static void crashWatchdogArmIfIdle(void) {
#ifdef HAVE_CRASH_WATCHDOG
    struct itimerspec cur;  // <<< BUG ANCHOR
    if (crash_watchdog_ok && timer_gettime(crash_watchdog_timer, &cur) == 0 &&
        (cur.it_value.tv_sec || cur.it_value.tv_nsec)) return;
#endif
