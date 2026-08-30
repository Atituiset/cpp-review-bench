/* KLEE 符号执行入口：符号化 digits/n/cap，驱动 imsi_bcd_encode 奇数长度越界读 + out 越界写。
 * 真实缺陷：n 奇数时 digits[i+1] 越界读；j>=cap 时 out[j] 越界写。
 */
#include <stdint.h>
#include <stddef.h>
#include <klee/klee.h>

size_t imsi_bcd_encode(const uint8_t *digits, size_t n, uint8_t *out, size_t cap);

int main(void)
{
    uint8_t digits[32];
    uint8_t out[32];
    size_t n, cap;
    klee_make_symbolic(digits, sizeof(digits), "digits");
    klee_make_symbolic(&n, sizeof(n), "n");
    klee_make_symbolic(&cap, sizeof(cap), "cap");
    imsi_bcd_encode(digits, n, out, cap);
    return 0;
}
