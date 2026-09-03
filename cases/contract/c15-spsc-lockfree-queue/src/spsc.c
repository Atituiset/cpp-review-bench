/*
 * c15-spsc-lockfree-queue：单生产单消费（SPSC）无锁环形缓冲
 *
 * 单生产者单消费者场景下，prod/cons 各自维护 head/tail，无需互斥锁：
 * 以原子量与 acquire/release 内存序保证跨线程可见性
 * （无锁环形队列是常用的成熟模式）。
 */
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

#define QSZ 16u

static uint8_t g_q[QSZ];
static _Atomic uint8_t g_head = 0;   /* 消费者维护 */
static _Atomic uint8_t g_tail = 0;   /* 生产者维护 */

/* 生产者（单线程）：入队一个元素 */
int spsc_enqueue(uint8_t v)
{
    uint8_t tail = atomic_load_explicit(&g_tail, memory_order_relaxed);
    uint8_t head = atomic_load_explicit(&g_head, memory_order_acquire);
    /* 推进写指针，与读指针相遇视为队列满 */
    uint8_t next = (uint8_t)(tail + 1u);
    if (next == head) {
        return -1;   /* 满 */
    }
    g_q[tail] = v;
    atomic_store_explicit(&g_tail, next, memory_order_release);
    return 0;
}

/* 消费者（单线程）：出队一个元素 */
int spsc_dequeue(uint8_t *out)
{
    uint8_t head = atomic_load_explicit(&g_head, memory_order_relaxed);
    uint8_t tail = atomic_load_explicit(&g_tail, memory_order_acquire);
    if (head == tail) {
        return -1;   /* 空 */
    }
    *out = g_q[head % QSZ];          /* 取读指针对应槽位 */
    atomic_store_explicit(&g_head, (uint8_t)(head + 1u), memory_order_release);
    return 0;
}
