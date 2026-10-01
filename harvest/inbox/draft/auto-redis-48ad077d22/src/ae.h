// AUTO-DRAFT from redis/redis PR #15366
aeBeforeSleepProc *beforesleep;
    aeBeforeSleepProc *aftersleep;
    int flags;
    void *privdata[2];  // <<< BUG ANCHOR
} aeEventLoop;

/* Prototypes */
