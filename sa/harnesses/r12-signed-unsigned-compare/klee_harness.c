/* KLEE 符号执行入口：符号化 len（有符号）/buf，驱动 r12_use 有符号/无符号混用负索引越界读。
 * 真实缺陷：len 为 int，与无符号 LIM 比较时转 size_t，len<0 变巨大正数，buf[len] 负索引越界（cwe-190）。
 */
#include <stdint.h>
#include <klee/klee.h>

void r12_use(int len, const uint8_t *buf);

int main(void)
{
    uint8_t buf[16];
    int len;
    klee_make_symbolic(buf, sizeof(buf), "buf");
    klee_make_symbolic(&len, sizeof(len), "len");
    r12_use(len, buf);
    return 0;
}
