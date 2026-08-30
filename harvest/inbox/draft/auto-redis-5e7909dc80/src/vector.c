// AUTO-DRAFT from redis/redis PR #15190
v->free = NULL;
}
  // <<< BUG ANCHOR
/* Reset the logical length to zero while preserving allocated storage. */
void vecClear(vec *v) {
    v->size = 0;
}


static int vecTestFreeCalls = 0;
static void vecTestFree(void *ptr) {
    UNUSED(ptr);
    vecTestFreeCalls++;
}

int vectorTest(int argc, char **argv, int flags)
    void *vstack2[2];
    vecInit(&v, vstack2, 2);
    vecSetFreeMethod(&v, vecTestFree);
    vecPush(&v, &one);
    vecPush(&v, &two);
    vecPush(&v, &three); /* triggers spill to heap */
    vecTestFreeCalls = 0;
    vecRelease(&v);
    test_cond("vecRelease() invokes free method on each element",
              vecTestFreeCalls == 3);

    vecInit(&v, NULL, 4);
    vecSetFreeMethod(&v, vecTestFree);
    vecTestFreeCalls = 0;
