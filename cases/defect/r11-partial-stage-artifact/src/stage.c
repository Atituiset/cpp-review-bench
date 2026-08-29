/* r11-partial-stage-artifact：评审过的树 ≠ 提交的树（构建/集成类缺陷）build */
#include <stdint.h>

/* 模拟「评审通过的源码」与「实际编译进镜像的源码」不一致：
 * 评审版函数有边界检查，提交版漏掉——集成/构建阶段缺陷。
 * 本例用宏开关模拟两种树，must_find 点指向「提交版漏检查」的代码。 */

#define STAGED_TREE 0   /* 0=提交版（漏检查），1=评审版（有检查） */

uint8_t r11_get(const uint8_t *arr, uint8_t idx)
{
#if STAGED_TREE
    if (idx >= 16u) return 0;        /* 评审版：有检查 */
#endif
    /* 锚点（must_find）：提交版（STAGED_TREE=0）此处无边界检查，
       arr[idx] 越界——评审过的树与提交的树不一致（build/集成类缺陷） */
    return arr[idx];
}
