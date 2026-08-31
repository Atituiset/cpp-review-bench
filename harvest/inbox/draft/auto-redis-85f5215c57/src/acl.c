// AUTO-DRAFT from redis/redis PR #13637
int flags = 0;
        size_t offset = 1;
        if (op[0] == '%') {
            for (; offset < oplen; offset++) {
                if (toupper(op[offset]) == 'R' && !(flags & ACL_READ_PERMISSION)) {
                    flags |= ACL_READ_PERMISSION;
                } else if (toupper(op[offset]) == 'W' && !(flags & ACL_WRITE_PERMISSION)) {
                    flags |= ACL_WRITE_PERMISSION;
                } else if (op[offset] == '~' && flags) {
                    offset++;
                    break;
                } else {
                    errno = EINVAL;  // <<< BUG ANCHOR
                    return C_ERR;
                }
            }
        } else {
            flags = ACL_ALL_PERMISSION;
        }
