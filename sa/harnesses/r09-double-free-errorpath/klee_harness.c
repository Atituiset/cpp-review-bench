/* KLEE 符号执行入口：符号化 sz，驱动 r09_open 错误路径释放后落入末尾再次释放（双重释放）。
 * 真实缺陷：sz>65535 时错误路径 free 后未 return，末尾再次 free → 双重释放（cwe-415）。
 */
#include <stdint.h>
#include <stdlib.h>
#include <klee/klee.h>

typedef struct { uint8_t *buf; } Ctx;

int r09_open(Ctx *c, uint32_t sz);

int main(void)
{
    Ctx c;
    uint32_t sz;
    c.buf = NULL;
    klee_make_symbolic(&sz, sizeof(sz), "sz");
    r09_open(&c, sz);
    return 0;
}
