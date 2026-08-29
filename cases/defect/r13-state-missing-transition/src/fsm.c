/* r13-state-missing-transition：状态机合法事件无 handler（消息枚举有、表中没有）logic */
#include <stdint.h>

typedef enum { ST_INIT = 0, ST_READY = 1, ST_RUN = 2, ST_MAX } State;

/* 事件枚举全集 */
typedef enum { EV_A = 0, EV_B = 1, EV_C = 2, EV_MAX } Event;

/* 状态表：EV_C 在枚举中合法，但表中无对应 handler（缺转换） */
typedef void (*handler_t)(void);
static handler_t g_table[ST_MAX][EV_MAX] = {
    [ST_INIT]  = { [EV_A] = 0, [EV_B] = 0 },          /* 缺 EV_C */
    [ST_READY] = { [EV_A] = 0, [EV_B] = 0 },          /* 缺 EV_C */
    [ST_RUN]   = { [EV_A] = 0, [EV_B] = 0 },          /* 缺 EV_C */
};

/* 分发：事件合法但无 handler，逻辑缺陷（logic） */
handler_t r13_lookup(State s, Event e)
{
    /* 锚点（must_find）：e 为合法枚举值 EV_C，但 g_table[s][e] 未初始化（NULL），
       调用方取到 NULL handler——状态机缺转换逻辑缺陷（logic） */
    return g_table[s][e];
}
