/* KLEE 符号执行入口：把长度与源缓冲符号化，让 KLEE 探索 i<len+1 多拷一字节的越界写。
 * 真实缺陷：r02_copy 的 for(i=0;i<len+1;i++) 比正确守卫多一字节，dst[len] 越界写。
 */
#include <stdint.h>
#include <klee/klee.h>
#include "parse.h"

int main(void)
{
    uint8_t dst[64];
    uint8_t src[64];
    uint8_t len;
    klee_make_symbolic(&len, sizeof(len), "len");
    klee_make_symbolic(src, sizeof(src), "src");
    r02_copy(dst, src, len);
    return 0;
}
