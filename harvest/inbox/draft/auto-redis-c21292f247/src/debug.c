// AUTO-DRAFT from redis/redis PR #14661
addReplyError(c,"Wrong protocol type name. Please use one of the following: string|integer|double|bignum|null|array|set|map|attrib|push|verbatim|true|false");
        }
    } else if (!strcasecmp(c->argv[1]->ptr,"sleep") && c->argc == 3) {
        double dtime = fast_float_strtod(c->argv[2]->ptr,NULL);  // <<< BUG ANCHOR
        long long utime = dtime*1000000;
        struct timespec tv;
