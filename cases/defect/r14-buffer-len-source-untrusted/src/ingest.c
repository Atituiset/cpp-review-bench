/* r14-buffer-len-source-untrusted：外部长度字段未约束即用于 memcpy cwe-787 */
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#define R14_BUF 48u

/* 网络报文头里的 len 字段来自外部，未校验即 memcpy，越界写 */
void r14_ingest(const uint8_t *pkt, uint8_t hdr_len, const uint8_t *payload)
{
    uint8_t buf[R14_BUF];
    /* 锚点（must_find）：hdr_len 来自外部报文未约束，memcpy 进 buf[R14_BUF]
       在 hdr_len>R14_BUF 时越界写（cwe-787） */
    memcpy(buf, payload, hdr_len);
    (void)pkt; (void)buf[0];
}

/* 安全点（must_not_find）：hdr_len 受界后拷贝 */
void r14_ingest_ok(const uint8_t *pkt, uint8_t hdr_len, const uint8_t *payload)
{
    uint8_t buf[R14_BUF];
    if (hdr_len > R14_BUF) {
        return;
    }
    memcpy(buf, payload, hdr_len);   /* 锚点（must_not_find）：受界，安全 */
    (void)pkt; (void)buf[0];
}
