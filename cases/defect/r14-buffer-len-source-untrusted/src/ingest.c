/* r14-buffer-len-source-untrusted：按报文头长度字段接收 payload */
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#define R14_BUF 48u

/* hdr_len 取自网络报文头，按该长度接收 payload */
void r14_ingest(const uint8_t *pkt, uint8_t hdr_len, const uint8_t *payload)
{
    uint8_t buf[R14_BUF];
    /* hdr_len 来自报文头长度字段 */
    memcpy(buf, payload, hdr_len);
    (void)pkt; (void)buf[0];
}

/* 接收前先按缓冲容量限制 hdr_len */
void r14_ingest_ok(const uint8_t *pkt, uint8_t hdr_len, const uint8_t *payload)
{
    uint8_t buf[R14_BUF];
    if (hdr_len > R14_BUF) {
        return;
    }
    memcpy(buf, payload, hdr_len);   /* 按受界后的长度拷贝 */
    (void)pkt; (void)buf[0];
}
