/*
 * r01-wrap-resume-bug：序号回绕时超时恢复选错点（RLC 式，跨函数逻辑）
 *
 * 真实缺陷：发送窗口维护 tx_next（uint8_t 模 256）。超时恢复时，基于
 * (tx_next - ack) 计算重传起点，但 ack 与 tx_next 跨回绕边界时，差值回绕为负，
 * 恢复选错点（漏发或重发错位）。
 *
 * must_find：tx_next/ack 为 uint8_t，回绕边界处 (tx_next - ack) 回绕导致
 * 恢复点错乱（cwe-190）。
 */
#include <stdint.h>

#define AMAP_MOD 256u

typedef struct {
    uint8_t tx_next;   /* 下一个发送序号（模 256） */
    uint8_t ack;       /* 已确认序号 */
    uint8_t win[256];  /* 发送窗口缓冲 */
} RlcTx;

/* 超时恢复：基于 (tx_next - ack) 计算重传起点 */
uint8_t resume_point(RlcTx *t)
{
    /* 锚点（must_find）：tx_next/ack 为 uint8_t，回绕边界处差值回绕为巨大正数，
       重传起点选错（漏发或越界索引 win[resume]） */
    uint8_t delta = (uint8_t)(t->tx_next - t->ack);   /* 回绕导致 delta 错误 */
    uint8_t resume = (uint8_t)(t->ack + delta);       /* 恢复点错乱 */
    return t->win[resume];   /* resume 越界风险 */
}

/* 安全点（must_not_find）：单点赋值，无回绕语义 */
void rlc_fill(RlcTx *t, uint8_t sn, uint8_t v)
{
    if (sn < AMAP_MOD) {
        t->win[sn] = v;   /* 锚点（must_not_find）：sn 已受界，安全 */
    }
}
