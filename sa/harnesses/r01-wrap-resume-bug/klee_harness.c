/* KLEE 符号执行入口：符号化 RlcTx 窗口字段，驱动 resume_point 的越界读。
 * 真实缺陷：win 容量 64，模 256 序号 resume 直接索引 win[resume] 未取模，
 * resume >= 64 时越界读。结构体布局必须与 src/rlc.c 一致（win[64]）。
 */
#include <stdint.h>
#include <klee/klee.h>

typedef struct { uint8_t tx_next; uint8_t ack; uint8_t win[64]; } RlcTx;

uint8_t resume_point(RlcTx *t);

int main(void)
{
    RlcTx t;
    klee_make_symbolic(&t, sizeof(t), "t");
    resume_point(&t);
    return 0;
}
