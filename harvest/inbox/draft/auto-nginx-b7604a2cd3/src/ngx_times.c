// AUTO-DRAFT from nginx/nginx PR #1243
#if (NGX_HAVE_CLOCK_MONOTONIC)
    struct timespec  ts;
  // <<< BUG ANCHOR
#if defined(CLOCK_MONOTONIC_FAST)
    clock_gettime(CLOCK_MONOTONIC_FAST, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif

    sec = ts.tv_sec;
    msec = ts.tv_nsec / 1000000;
