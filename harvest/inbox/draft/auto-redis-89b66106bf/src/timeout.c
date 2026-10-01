// AUTO-DRAFT from redis/redis PR #15194
return C_ERR;
  // <<< BUG ANCHOR
        ftval *= 1000.0;  /* seconds => millisec */
        if (ftval > LLONG_MAX) {
            addReplyError(c, "timeout is out of range");
            return C_ERR;
        }
