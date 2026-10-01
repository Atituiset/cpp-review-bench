// AUTO-DRAFT from redis/redis PR #14877
* update the keysizes histogram. Otherwise, the histogram already 
         * updated in lookupStringForBitCommand() by calling dbAdd(). */
        if ((strOldSize > 0) && (strGrowSize != 0))
            updateKeysizesHist(c->db, getKeySlot(c->argv[1]->ptr), OBJ_STRING,   // <<< BUG ANCHOR
                               strOldSize, strOldSize + strGrowSize);
    }

    /* Return original value. */
         * update the keysizes histogram. Otherwise, the histogram already 
         * updated in lookupStringForBitCommand() by calling dbAdd(). */
        if ((strOldSize > 0) && (strGrowSize != 0))
            updateKeysizesHist(c->db, getKeySlot(c->argv[1]->ptr), OBJ_STRING,
                               strOldSize, strOldSize + strGrowSize);
        
        keyModified(c,c->db,c->argv[1],o,1);
        notifyKeyspaceEvent(NOTIFY_STRING,"setbit",c->argv[1],c->db->id);
