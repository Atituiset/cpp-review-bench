/* r04-oob-write-stack：无界拷贝进定长栈缓冲 cwe-787 */
#include <stdint.h>
#include <string.h>

#define STK_BUF 32u

/* 外部长度字段未约束即 memcpy 进栈缓冲，越界写 */
void r04_recv(const uint8_t *payload, uint8_t len)
{
    uint8_t buf[STK_BUF];
    /* 锚点（must_find）：len 来自外部未校验，memcpy 进 buf[STK_BUF] 越界写（cwe-787） */
    memcpy(buf, payload, len);
    (void)buf[0];
}

/* 安全点（must_not_find）：len 受界后拷贝 */
void r04_recv_ok(const uint8_t *payload, uint8_t len)
{
    uint8_t buf[STK_BUF];
    if (len > STK_BUF) {
        return;
    }
    memcpy(buf, payload, len);   /* 锚点（must_not_find）：len 受界，安全 */
    (void)buf[0];
}
