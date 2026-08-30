/* KLEE 符号执行入口：把外部长度字段符号化，让 KLEE 探索越界写路径。
 * 真实缺陷：r04_recv 的 memcpy 进栈缓冲未校验 len，len 符号化后 KLEE 能触发越界写。
 */
#include <stdint.h>
#include <stdlib.h>
#include <klee/klee.h>
#include "recv.h"

int main(void)
{
    uint8_t payload[128];
    uint8_t len;
    klee_make_symbolic(&len, sizeof(len), "len");
    klee_make_symbolic(payload, sizeof(payload), "payload");
    r04_recv(payload, len);
    return 0;
}
