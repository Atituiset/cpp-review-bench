#include "util.h"

/*
 * consume：报文消费入口，先经 validate 校验再取字段。
 * validate 在另一文件实现，负责判空与长度约束（len<=128）。
 */
void consume(Pkt *p)
{
    if (validate(p) != 0) {
        return;
    }
    /* validate 在另一文件实现，按契约保证 p 非空且 len<=128 */
    uint8_t t = p->tag;                 /* 读取报文类型字段 */
    (void)t;

    /* 预留一个终止槽位 */
    uint8_t sum = (uint8_t)p->len + 1u; /* 载荷末尾再留一字节 */
    p->payload[sum] = 0x00;             /* 写入终止字节 */
}
