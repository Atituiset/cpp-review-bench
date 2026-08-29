/*
 * c12-intended-wrap-seq：有意的序号回绕模运算（契约声明）
 *
 * 真实形态：协议序号按 8/16 位回绕是设计意图（如 RLC 序号 mod 256），
 * 评审者/工具看到 `seq++` 或 `seq + 1` 容易误报 cwe-190 整数回绕。
 * 但回绕是契约行为，下游按模运算使用——安全。
 *
 * 混入真实缺陷：回绕后用于索引时少一次模约（cwe-787 越界）。
 */
#include <stdint.h>

#define SEQ_MOD 256u

/* 序号推进：回绕是设计意图（契约声明） */
uint8_t seq_next(uint8_t seq)
{
    /* 锚点（must_not_find）：seq+1 回绕是预期行为，非缺陷 */
    return (uint8_t)(seq + 1u);
}

/* 用序号索引环形缓冲：混入真实越界缺陷 */
uint8_t ring_get(uint8_t *ring, uint8_t seq, uint8_t idx)
{
    /* 回绕是设计意图，但下方缺少对 idx 的模约 */
    uint8_t pos = (uint8_t)(seq + idx);   /* 锚点（must_find）：pos 未对 SEQ_MOD 约简 */
    return ring[pos];                      /* idx 大时 pos 越界 ring[256] */
}
