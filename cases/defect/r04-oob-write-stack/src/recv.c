/* r04-oob-write-stack：把外部报文接收到栈缓冲 */
#include <stdint.h>
#include <string.h>

#define STK_BUF 32u

/* 按报文自带长度字段接收进栈缓冲 */
void r04_recv(const uint8_t *payload, uint8_t len)
{
    uint8_t buf[STK_BUF];
    /* len 取自报文头长度字段 */
    memcpy(buf, payload, len);
    (void)buf[0];
}

/* 接收前先按缓冲容量限制长度 */
void r04_recv_ok(const uint8_t *payload, uint8_t len)
{
    uint8_t buf[STK_BUF];
    if (len > STK_BUF) {
        return;
    }
    memcpy(buf, payload, len);   /* 按受界后的长度拷贝 */
    (void)buf[0];
}
