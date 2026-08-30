/* KLEE 符号执行入口：符号化 val，驱动 r06_fill 的 i<=R06_N 循环越界写。
 * 真实缺陷：循环条件 i<=R06_N 比数组容量多一，arr[R06_N] 越界写（cwe-787）。
 */
#include <stdint.h>
#include <klee/klee.h>

void r06_fill(uint8_t *arr, uint8_t val);

int main(void)
{
    uint8_t arr[16];   /* 容量 16，循环写入索引 0..16 → arr[16] 越界 */
    uint8_t val;
    klee_make_symbolic(&val, sizeof(val), "val");
    r06_fill(arr, val);
    return 0;
}
