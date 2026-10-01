// AUTO-DRAFT from redis/redis PR #15108
/* Encode last 4-char into 32-bit serialized class ID (24b name + 5b version + 3b flags) */
    uint32_t encName4chars = 0;
    for (int j = 0; j < KM_FULLNAME_LEN; j++) {
        char *p = strchr(keyMetaCharSet, fullname[j]);  // <<< BUG ANCHOR
        if (!p) return 0; /* Invalid character in name */
        unsigned long pos = p - keyMetaCharSet;
        encName9Chars = (encName9Chars << KM_ENC_CHAR_BITS) | pos;
