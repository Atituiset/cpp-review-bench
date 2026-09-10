// AUTO-DRAFT from redis/redis PR #15408
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdint.h>

    int64_t flags = 0;
    for (int j = 0; j < cmd->key_specs_num; j++) {
        keySpec *spec = cmd->key_specs + j;
        flags |= inv? ~spec->flags : spec->flags;
    }
    return flags;
