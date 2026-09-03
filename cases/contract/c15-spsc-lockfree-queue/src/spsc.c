/*
 * c15-spsc-lockfree-queue：单生产单消费（SPSC）无锁环形缓冲
 *
 * 单生产者单消费者场景下，prod/cons 各自维护 head/tail，无需加锁
 * （无锁环形队列是常用的成熟模式）。
 */
#include <stdint.h>
#include <stddef.h>

#define QSZ 16u

static uint8_t  g_q[QSZ];
static uint8_t  g_head = 0;   /* 消费者维护 */
static uint8_t  g_tail = 0;   /* 生产者维护 */

/* 生产者（单线程）：入队一个元素 */
int spsc_enqueue(uint8_t v)
{
    /* 推进写指针，与读指针相遇视为队列满 */
    uint8_t next = (uint8_t)(g_tail + 1u);
    if (next == g_head) {
        return -1;   /* 满 */
    }
    g_q[g_tail] = v;
    g_tail = next;
    return 0;
}

/* 消费者（单线程）：出队一个元素 */
int spsc_dequeue(uint8_t *out)
{
    if (g_head == g_tail) {
        return -1;   /* 空 */
    }
    *out = g_q[g_head];          /* 取读指针对应槽位 */
    g_head = (uint8_t)(g_head + 1u);
    return 0;
}
