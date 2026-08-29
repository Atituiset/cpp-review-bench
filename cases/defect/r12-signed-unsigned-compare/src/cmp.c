/* r12-signed-unsigned-compare：有符号/无符号混用致边界检查失效 cwe-190 */
#include <stdint.h>

#define LIM 16

/* len 为有符号，与无符号 LIM 比较时隐式转换，负 len 绕过检查 */
void r12_use(int len, const uint8_t *buf)
{
    /* 锚点（must_find）：len 为 int，与无符号 LIM 比较时转为 size_t，
       len<0 时变成巨大正数，检查 `len < LIM` 通过，buf[len] 负索引越界（cwe-190） */
    if (len < LIM) {
        (void)buf[len];   /* len 为负时越界 */
    }
}

/* 安全点（must_not_find）：先有符号范围检查 */
void r12_use_ok(int len, const uint8_t *buf)
{
    if (len >= 0 && len < LIM) {   /* 锚点（must_not_find）：有符号检查在前，安全 */
        (void)buf[len];
    }
}
