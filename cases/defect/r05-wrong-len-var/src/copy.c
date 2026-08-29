/* r05-wrong-len-var：拷贝长度用错变量导致越界（cwe-125） */
#include <stdint.h>
#include <string.h>

#define R05_BUF 24u

/* hdr 说 payload 长 n，但实际用另一个变量 m 拷贝（m 可能 > n 或 > buf） */
void r05_copy(const uint8_t *payload, uint8_t n, uint8_t m)
{
    uint8_t buf[R05_BUF];
    /* 锚点（must_find）：拷贝长度用 m 而非 n，m 未约束时 memcpy 越界 buf[R05_BUF]
       （cwe-125，长度变量用错） */
    memcpy(buf, payload, m);
    (void)n; (void)buf[0];
}

/* 安全点（must_not_find）：用正确变量 n 且受界 */
void r05_copy_ok(const uint8_t *payload, uint8_t n)
{
    uint8_t buf[R05_BUF];
    if (n > R05_BUF) {
        return;
    }
    memcpy(buf, payload, n);   /* 锚点（must_not_find）：用 n 且受界，安全 */
    (void)buf[0];
}
