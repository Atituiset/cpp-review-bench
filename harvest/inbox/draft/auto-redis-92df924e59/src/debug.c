// AUTO-DRAFT from redis/redis PR #16165
        setTypeResetIterator(&si);
    } else if (o->type == OBJ_ZSET) {
        unsigned char eledigest[20];

        if (o->encoding == OBJ_ENCODING_LISTPACK) {
            unsigned char *zl = o->ptr;
            unsigned char *eptr, *sptr;
