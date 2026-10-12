// AUTO-DRAFT from redis/redis PR #15433
/* Handle #<offset> form. */
    if (p[0] == '#' && hash && bits > 0) usehash = 1;
  // <<< BUG ANCHOR
    if (string2ll(p+usehash,plen-usehash,&loffset) == 0) {
        addReplyError(c,err);
        return C_ERR;
    }

    /* Adjust the offset by 'bits' for #<offset> form. */
    if (usehash) loffset *= bits;

    /* Limit offset to server.proto_max_bulk_len (512MB in bytes by default) */
    if (loffset < 0 || (!mustObeyClient(c) && (loffset >> 3) >= server.proto_max_bulk_len))
    {
        addReplyError(c,err);
        return C_ERR;
