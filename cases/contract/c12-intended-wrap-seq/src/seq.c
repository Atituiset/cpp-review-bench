/*
 * c12-intended-wrap-seq：协议序号按 8 位模运算回绕
 *
 * 协议序号按 8/16 位回绕是设计意图（如 RLC 序号 mod 256），
 * 下游一律按模运算使用序号。
 */
#include <stdint.h>

#define SEQ_MOD 256u

/* 序号推进：按 8 位模 256 回绕 */
uint8_t seq_next(uint8_t seq)
{
    return (uint8_t)(seq + 1u);
}

/* 用序号加偏移索引环形缓冲 */
uint8_t ring_get(uint8_t *ring, uint8_t seq, uint8_t idx)
{
    /* 基准序号加上窗口内偏移得到读取位置 */
    uint8_t pos = (uint8_t)(seq + idx);   /* 窗口内读取位置 */
    return ring[pos];                      /* 取环形缓冲对应槽位 */
}
