/*
 * r01-wrap-resume-bug：序号回绕时超时恢复选错点（RLC 式，跨函数逻辑）
 *
 * RLC 发送窗口维护模 256 的发送序号，窗口缓冲按 WIN_SIZE 环形分配。
 * 超时恢复时基于 (tx_next - ack) 计算重传起点，再从窗口缓冲取回对应
 * 序号的缓存负载。
 */
#include <stdint.h>

#define AMAP_MOD 256u
#define WIN_SIZE 64u

typedef struct {
    uint8_t tx_next;          /* 下一个发送序号（模 256） */
    uint8_t ack;              /* 已确认序号 */
    uint8_t win[WIN_SIZE];    /* 发送窗口缓冲（环形，容量 64） */
} RlcTx;

/* 超时恢复：基于 (tx_next - ack) 计算重传起点 */
uint8_t resume_point(RlcTx *t)
{
    uint8_t delta = (uint8_t)(t->tx_next - t->ack);
    uint8_t resume = (uint8_t)(t->ack + delta);
    return t->win[resume];
}

/* 按序号写入窗口缓存 */
void rlc_fill(RlcTx *t, uint8_t sn, uint8_t v)
{
    t->win[sn % WIN_SIZE] = v;
}
