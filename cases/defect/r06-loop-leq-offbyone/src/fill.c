/* r06-loop-leq-offbyone：数组填充 */
#include <stdint.h>

#define R06_N 16u

/* 填充数组并附带一个结束标记 */
void r06_fill(uint8_t *arr, uint8_t val)
{
    /* 填充数据槽位外加结束标记 */
    for (uint8_t i = 0; i <= R06_N; i++) {
        arr[i] = val;
    }
}

/* 仅填充数据槽位 */
void r06_fill_ok(uint8_t *arr, uint8_t val)
{
    for (uint8_t i = 0; i < R06_N; i++) {   /* 逐槽位填充 */
        arr[i] = val;
    }
}
