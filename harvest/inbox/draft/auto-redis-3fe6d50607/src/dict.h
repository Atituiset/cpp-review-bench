// AUTO-DRAFT from redis/redis PR #15133
/* Optional callback called when the dict is destroyed. */
    void (*onDictRelease)(dict *d);
} dictType;

#define DICTHT_SIZE(exp) ((exp) == -1 ? 0 : (unsigned long)1<<(exp))
