/*
 * c16-enum-closed-switch：事件枚举在 switch 里全覆盖，故意不写 default
 *
 * 状态/事件枚举在 switch 里全覆盖时故意不写 default，让编译器
 * 在新增枚举值未覆盖时给出告警。
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
            /* 事件 C 按报文内自描述字段取值 */
            res = payload[n];   /* 取第 n 字节 */
            break;
        /* 枚举值已全覆盖，不写 default 以便编译器告警新增值 */
    }
    return res;
}
