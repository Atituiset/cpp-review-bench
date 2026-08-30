// AUTO-DRAFT from redis/redis PR #15150
if (entry->type != REDIS_REPLY_ARRAY || entry->elements < 4 ||
            entry->element[0]->type != REDIS_REPLY_STRING ||
            entry->element[1]->type != REDIS_REPLY_INTEGER ||
            entry->element[3]->type != REDIS_REPLY_INTEGER) return;  // <<< BUG ANCHOR
        char *cmdname = entry->element[0]->str;
        int i;
