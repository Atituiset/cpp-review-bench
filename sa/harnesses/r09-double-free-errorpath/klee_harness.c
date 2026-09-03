/* KLEE 符号执行入口：驱动 r09_open 错误路径释放后落入末尾再次释放（双重释放）。
 * sz 取大于 65535 的具体值：符号化 sz 会让 KLEE 在 malloc 处报
 * concretized symbolic size 提前终止，触达不了 double-free 路径。
 */
#include <stdint.h>
#include <stdlib.h>
#include <klee/klee.h>

typedef struct { uint8_t *buf; } Ctx;

int r09_open(Ctx *c, uint32_t sz);

int main(void)
{
    Ctx c;
    c.buf = NULL;
    r09_open(&c, 70000u);
    return 0;
}
