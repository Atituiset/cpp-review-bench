/* KLEE 符号执行入口：符号化 n/size，驱动 r07_alloc 的乘法回绕→小分配→越界写。
 * 真实缺陷：n*size 为 uint32 先乘后转 size_t，回绕成小值，malloc 不足，循环越界写（cwe-190+787）。
 */
#include <stdint.h>
#include <stdlib.h>
#include <klee/klee.h>

void *r07_alloc(uint32_t n, uint32_t size);

int main(void)
{
    uint32_t n, size;
    klee_make_symbolic(&n, sizeof(n), "n");
    klee_make_symbolic(&size, sizeof(size), "size");
    void *p = r07_alloc(n, size);
    if (p) free(p);
    return 0;
}
