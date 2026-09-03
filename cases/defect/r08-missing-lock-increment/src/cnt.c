/* r08-missing-lock-increment：多线程共享计数器 */
#include <stdint.h>

#define CNT_MAX 1000u

static uint32_t g_counter = 0;

/* 各工作线程完成一个任务后递增计数 */
void r08_inc(void)
{
    /* 任务计数加一 */
    g_counter++;
    (void)g_counter;
}

/* 读取当前计数 */
uint32_t r08_read(void)
{
    return g_counter;   /* 当前计数 */
}
