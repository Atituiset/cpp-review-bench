/* KLEE 符号执行入口：符号化 payload/n/m，驱动 r05_copy 用错长度变量 memcpy 越界。
 * 真实缺陷：拷贝长度用 m 而非 n，m 未约束时 memcpy 越界 buf[R05_BUF]（cwe-125）。
 */
#include <stdint.h>
#include <klee/klee.h>

void r05_copy(const uint8_t *payload, uint8_t n, uint8_t m);

int main(void)
{
    uint8_t payload[64];
    uint8_t n, m;
    klee_make_symbolic(payload, sizeof(payload), "payload");
    klee_make_symbolic(&n, sizeof(n), "n");
    klee_make_symbolic(&m, sizeof(m), "m");
    r05_copy(payload, n, m);
    return 0;
}
