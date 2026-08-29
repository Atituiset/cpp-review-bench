/* r08-missing-lock-increment：多线程计数器无锁递增 cwe-362 */
#include <stdint.h>

#define CNT_MAX 1000u

static uint32_t g_counter = 0;

/* 两个线程并发调此函数，无锁递增存在数据竞争（cwe-362） */
void r08_inc(void)
{
    /* 锚点（must_find）：g_counter++ 非原子，并发读写竞争导致丢失更新（cwe-362） */
    g_counter++;
    (void)g_counter;
}

/* 安全点（must_not_find）：单线程顺序读取，无竞争 */
uint32_t r08_read(void)
{
    return g_counter;   /* 锚点（must_not_find）：单线程读，无竞争 */
}
