/* r05-wrong-len-var：按报文长度字段拷贝 payload */
#include <stdint.h>
#include <string.h>

#define R05_BUF 24u

/* hdr 声明 payload 长 n，实际拷贝长度取 m */
void r05_copy(const uint8_t *payload, uint8_t n, uint8_t m)
{
    uint8_t buf[R05_BUF];
    /* 拷贝长度取 m */
    memcpy(buf, payload, m);
    (void)n; (void)buf[0];
}

/* 按声明长度 n 拷贝，先按缓冲容量限制 */
void r05_copy_ok(const uint8_t *payload, uint8_t n)
{
    uint8_t buf[R05_BUF];
    if (n > R05_BUF) {
        return;
    }
    memcpy(buf, payload, n);   /* 按受界后的长度拷贝 */
    (void)buf[0];
}
