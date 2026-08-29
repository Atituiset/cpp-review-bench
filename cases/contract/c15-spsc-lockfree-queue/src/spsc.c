/*
 * c15-spsc-lockfree-queue：单生产单消费（SPSC）无锁环形缓冲
 *
 * 真实形态：单生产者单消费者场景下，prod/cons 各自维护 head/tail，无需加锁
 * （无锁环形队列是成熟正确模式）。工具看到「无锁访问共享 head/tail」可能误报
 * cwe-362 数据竞争——但 SPSC 语义下无竞争，安全。
 *
 * 混入真实缺陷：enqueue 时未检查队列满，tail 回绕后覆盖未消费数据导致越界写。
 */
#include <stdint.h>
#include <stddef.h>

#define QSZ 16u

static uint8_t  g_q[QSZ];
static uint8_t  g_head = 0;   /* 消费者维护 */
static uint8_t  g_tail = 0;   /* 生产者维护 */

/* 生产者（单线程）：无锁入队，SPSC 语义下安全 */
int spsc_enqueue(uint8_t v)
{
    /* 锚点（must_find）：未检查队列满，连续生产 QSZ 次后 tail 回绕覆盖未消费数据，
       且 g_q[tail] 写越界（tail 不约束 < QSZ 时） */
    uint8_t next = (uint8_t)(g_tail + 1u);
    if (next == g_head) {
        return -1;   /* 满 */
    }
    g_q[g_tail] = v;
    g_tail = next;
    return 0;
}

/* 消费者（单线程）：无锁出队 */
int spsc_dequeue(uint8_t *out)
{
    if (g_head == g_tail) {
        return -1;   /* 空 */
    }
    *out = g_q[g_head];          /* 锚点（must_not_find）：SPSC 下 g_head 由消费者独占维护，安全 */
    g_head = (uint8_t)(g_head + 1u);
    return 0;
}
