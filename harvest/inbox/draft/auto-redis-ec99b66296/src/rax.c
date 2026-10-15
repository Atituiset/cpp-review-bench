// AUTO-DRAFT from redis/redis PR #15508
raxNode *n = it->node;
    while(steps > 0 || (!n->iskey && it->leaf_slot_idx < 0)) {
        int numchildren = n->iscompr ? 1 : n->size;
        int r = rand() % (numchildren+(n != it->rt->head));
  // <<< BUG ANCHOR
        if (r == numchildren) {
            /* Go up: if parked on a virtual leaf, exit it (the leaf parent
             * stays as n). Otherwise pop the real parent. Either way, the
             * edge to strip is owned by the resulting `n`. */
            if (it->leaf_slot_idx >= 0)
                it->leaf_slot_idx = -1;
            else
                n = raxStackPop(&it->stack);
            int todel = n->iscompr ? n->size : 1;
            raxIteratorDelChars(it,todel);
        } else {
        raxFree(r);
    }

    TEST("inline-leaf: delete triggers recompression into a sibling leaf parent") {
        /* k1 and k2 diverge at byte 1, each with a long unique suffix:
         *   "A" -> {A,B branch} -> "AAAAAA"(leaf parent, v1)   = k1
