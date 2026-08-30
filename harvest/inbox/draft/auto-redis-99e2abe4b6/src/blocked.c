// AUTO-DRAFT from redis/redis PR #15594
if (de) {
        list *clients = dictGetVal(de);
        listNode *ln;
        listIter li;
        listRewind(clients,&li);

        /* Avoid processing more than the initial count so that we're not stuck
         * in an endless loop in case the reprocessing of the command blocks again. */
        long count = listLength(clients);
        while ((ln = listNext(&li)) && count--) {
            client *receiver = listNodeValue(ln);
            kvobj *o = lookupKeyReadWithFlags(rl->db, rl->key, LOOKUP_NOEFFECTS);
            /* 1. In case new key was added/touched we need to verify it satisfy the
