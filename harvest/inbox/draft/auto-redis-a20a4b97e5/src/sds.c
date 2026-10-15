// AUTO-DRAFT from redis/redis PR #15171
unsigned char *fp; /* flags pointer. */
    size_t usable;
  // <<< BUG ANCHOR
    assert(initlen + hdrlen + 1 > initlen); /* Catch size_t overflow */
    sh = trymalloc?
        s_trymalloc_usable(hdrlen+initlen+1, &usable) :
        s_malloc_usable(hdrlen+initlen+1, &usable);
