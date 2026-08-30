// AUTO-DRAFT from redis/redis PR #14888
#define LISTPACK_MAX_SAFETY_SIZE (1<<30)
int lpSafeToAdd(unsigned char* lp, size_t add) {
    size_t len = lp? lpGetTotalBytes(lp): 0;
    if (len + add > LISTPACK_MAX_SAFETY_SIZE)  // <<< BUG ANCHOR
        return 0;
    return 1;
}
