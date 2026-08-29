/*
 * c16-enum-closed-switch：枚举全集闭合、故意无 default
 *
 * 真实形态：状态/事件枚举在 switch 里全覆盖，故意不写 default（让编译器
 * 在未覆盖新增枚举值时告警）。工具可能按「switch 无 default 可能漏处理」误报
 * cwe-unknown/逻辑缺陷——但枚举闭合，无 default 是设计意图，安全。
 *
 * 混入真实缺陷：某 case 里对 payload 索引缺少边界（cwe-125 越界读）。
 */
#include <stdint.h>

typedef enum {
    EVT_A = 0,
    EVT_B = 1,
    EVT_C = 2
} Evt;   /* 枚举闭合：仅 A/B/C */

uint8_t handle(Evt e, const uint8_t *payload, uint8_t n)
{
    uint8_t res = 0;
    switch (e) {
        case EVT_A:
            res = payload[0];
            break;
        case EVT_B:
            res = payload[1];
            break;
        case EVT_C:
            /* 锚点（must_find）：n 未校验即用于索引，n>=3 时 payload[n] 越界 */
            res = payload[n];   /* 真实越界读缺陷 */
            break;
        /* 无 default：枚举闭合，设计意图，非缺陷（must_not_find 点见 notes） */
    }
    return res;
}
