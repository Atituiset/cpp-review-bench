/* r02-offby-one-guard：报文 payload 拷贝到定长缓冲 */
#include <stdint.h>
#include <string.h>

#define MAX_PAYLOAD 64u

/* 拷贝报文 payload（含结尾标志字节）到定长缓冲 */
void r02_copy(uint8_t *dst, const uint8_t *src, uint8_t len)
{
    if (len > MAX_PAYLOAD) {
        return;   /* 长度上限检查 */
    }
    /* 连数据带结尾标志一起拷 */
    for (uint8_t i = 0; i < len + 1u; i++) {
        dst[i] = src[i];
    }
}

/* 仅拷贝 payload 数据部分 */
void r02_copy_ok(uint8_t *dst, const uint8_t *src, uint8_t len)
{
    if (len > MAX_PAYLOAD) {
        return;
    }
    for (uint8_t i = 0; i < len; i++) {   /* 逐字节拷贝 */
        dst[i] = src[i];
    }
}
