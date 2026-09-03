// AUTO-DRAFT from redis/redis PR #15071
/* Free an sds string. No operation is performed if 's' is NULL. */
void sdsfree(sds s) {
    if (s == NULL) return;  // <<< BUG ANCHOR
    s_free((char*)s-sdsHdrSize(s[-1]));
}

void sdsfreeusable(sds s, size_t *usable) {
    if (s == NULL) return;
    s_free_usable((char*)s-sdsHdrSize(s[-1]), usable);
}

/* Generic version of sdsfree. */
