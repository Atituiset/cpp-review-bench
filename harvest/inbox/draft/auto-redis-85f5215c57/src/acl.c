// AUTO-DRAFT from redis/redis PR #13637
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
  // <<< BUG ANCHOR
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
                    errno = EINVAL;
                    return C_ERR;
                }
            }
        } else {
            flags = ACL_ALL_PERMISSION;
        }
