// AUTO-DRAFT from redis/redis PR #16371
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
        } else {
            /* The key's allocation size reflects the entire robj allocation.  
             * For embedded values, report an allocation size of 0. */
            size_t obj_alloc = zmalloc_usable_size(val);
            size_t val_alloc = val->encoding == OBJ_ENCODING_RAW ? sdsAllocSize(val->ptr) : 0;
            addReplyStatusFormat(c,
                "key_sds_len:%lld, key_sds_avail:%lld, key_zmalloc: %lld, "
