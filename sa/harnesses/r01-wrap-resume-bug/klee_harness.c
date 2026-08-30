/* KLEE 符号执行入口：符号化 RlcTx 窗口字段，驱动 resume_point 的回绕越界读。
 * 真实缺陷：tx_next/ack 为 uint8_t，回绕边界处 (tx_next-ack) 回绕，win[resume] 越界。
 */
#include <stdint.h>
#include <klee/klee.h>

typedef struct { uint8_t tx_next; uint8_t ack; uint8_t win[256]; } RlcTx;

uint8_t resume_point(RlcTx *t);

int main(void)
{
    RlcTx t;
    klee_make_symbolic(&t, sizeof(t), "t");
    resume_point(&t);
    return 0;
}
