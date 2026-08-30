/* KLEE 符号执行入口：符号化 pkt/hdr_len/payload，驱动 r14_ingest 外部长度未校验 memcpy 越界写。
 * 真实缺陷：hdr_len 来自外部报文未约束，memcpy 进 buf[R14_BUF] 在 hdr_len>R14_BUF 时越界写（cwe-787）。
 */
#include <stdint.h>
#include <klee/klee.h>

void r14_ingest(const uint8_t *pkt, uint8_t hdr_len, const uint8_t *payload);

int main(void)
{
    uint8_t pkt[64];
    uint8_t payload[64];
    uint8_t hdr_len;
    klee_make_symbolic(pkt, sizeof(pkt), "pkt");
    klee_make_symbolic(payload, sizeof(payload), "payload");
    klee_make_symbolic(&hdr_len, sizeof(hdr_len), "hdr_len");
    r14_ingest(pkt, hdr_len, payload);
    return 0;
}
