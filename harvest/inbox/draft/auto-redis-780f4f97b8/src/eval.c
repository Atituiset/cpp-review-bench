// AUTO-DRAFT from redis/redis PR #15115
for (j = 0; j < ldb.bpcount; j++) {
        if (ldb.bp[j] == line) {
            ldb.bpcount--;
            memmove(ldb.bp+j,ldb.bp+j+1,ldb.bpcount-j);  // <<< BUG ANCHOR
            return 1;
        }
    }
