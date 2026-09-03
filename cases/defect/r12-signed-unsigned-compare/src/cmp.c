/* r12-signed-unsigned-compare：按长度访问缓冲 */
#include <stdint.h>

#define LIM 16

/* 长度在上限内时取对应字节 */
void r12_use(int len, const uint8_t *buf)
{
    /* 长度上限检查 */
    if (len < LIM) {
        (void)buf[len];   /* 取第 len 字节 */
    }
}

/* 先做范围检查再取字节 */
void r12_use_ok(int len, const uint8_t *buf)
{
    if (len >= 0 && len < LIM) {   /* 范围检查 */
        (void)buf[len];
    }
}
