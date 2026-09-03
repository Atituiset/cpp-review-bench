/* r11-partial-stage-artifact：按构建宏切换的取数实现 */
#include <stdint.h>

/* 用宏开关在两种构建配置间切换：带范围钳制的版本与直取版本 */

#define STAGED_TREE 0   /* 0=直取版本，1=带范围钳制版本 */

uint8_t r11_get(const uint8_t *arr, uint8_t idx)
{
#if STAGED_TREE
    if (idx >= 16u) return 0;        /* 范围钳制 */
#endif
    /* 取第 idx 个元素 */
    return arr[idx];
}
