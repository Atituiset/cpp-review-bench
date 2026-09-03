/* r08-missing-lock-increment：多线程共享计数器
 *
 * 使用形态：若干工作线程并行运行，各线程每完成一个任务调一次 r08_inc；
 * 主线程在全部工作线程 join 之后再调 r08_read 汇总总数。
 */
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

/* 读取当前计数（约定：全部工作线程 join 之后由主线程调用） */
uint32_t r08_read(void)
{
    return g_counter;   /* 当前计数 */
}
