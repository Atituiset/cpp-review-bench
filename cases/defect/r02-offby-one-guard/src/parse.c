/* r02-offby-one-guard：长度守卫少算一字节（报文解析）cwe-125 */
#include <stdint.h>
#include <string.h>

#define MAX_PAYLOAD 64u

/* 拷贝报文 payload 到定长缓冲，守卫少算一字节导致越界写 */
void r02_copy(uint8_t *dst, const uint8_t *src, uint8_t len)
{
    if (len > MAX_PAYLOAD) {
        return;   /* 守卫：len 上限 */
    }
    /* 锚点（must_find）：循环条件 i < len+1 多拷一字节，dst[len] 越界（cwe-125） */
    for (uint8_t i = 0; i < len + 1u; i++) {
        dst[i] = src[i];
    }
}

/* 安全点（must_not_find）：正确的 i < len 守卫 */
void r02_copy_ok(uint8_t *dst, const uint8_t *src, uint8_t len)
{
    if (len > MAX_PAYLOAD) {
        return;
    }
    for (uint8_t i = 0; i < len; i++) {   /* 锚点（must_not_find）：i<len 正确受界 */
        dst[i] = src[i];
    }
}
