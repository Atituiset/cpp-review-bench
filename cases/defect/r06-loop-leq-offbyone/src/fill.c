/* r06-loop-leq-offbyone：循环条件用 <= 导致多越一字节（cwe-787） */
#include <stdint.h>

#define R06_N 16u

/* 写入 0..R06_N 共 R06_N+1 个元素到仅 R06_N 容量的数组 */
void r06_fill(uint8_t *arr, uint8_t val)
{
    /* 锚点（must_find）：i <= R06_N 比数组容量多一，arr[R06_N] 越界写（cwe-787） */
    for (uint8_t i = 0; i <= R06_N; i++) {
        arr[i] = val;
    }
}

/* 安全点（must_not_find）：i < R06_N 正确 */
void r06_fill_ok(uint8_t *arr, uint8_t val)
{
    for (uint8_t i = 0; i < R06_N; i++) {   /* 锚点（must_not_find）：i<R06_N 受界 */
        arr[i] = val;
    }
}
