// AUTO-DRAFT from redis/redis PR #15710
aeEventLoop *eventLoop;
    int i;
  // <<< BUG ANCHOR
    monotonicInit();    /* just in case the calling app didn't initialize */

    if ((eventLoop = zmalloc(sizeof(*eventLoop))) == NULL) goto err;
    eventLoop->nevents = setsize < INITIAL_EVENT ? setsize : INITIAL_EVENT;
