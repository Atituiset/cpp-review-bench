/* r13-state-missing-transition：状态机事件分发表 */
#include <stdint.h>

typedef enum { ST_INIT = 0, ST_READY = 1, ST_RUN = 2, ST_MAX } State;

/* 事件枚举全集 */
typedef enum { EV_A = 0, EV_B = 1, EV_C = 2, EV_MAX } Event;

/* 状态转换表：按状态与事件查 handler */
typedef void (*handler_t)(void);

static uint32_t g_ev_count = 0;   /* 已处理事件计数 */

static void h_init_a(void)  { g_ev_count++; }
static void h_init_b(void)  { g_ev_count++; }
static void h_ready_a(void) { g_ev_count++; }
static void h_ready_b(void) { g_ev_count++; }
static void h_run_a(void)   { g_ev_count++; }
static void h_run_b(void)   { g_ev_count++; }

static handler_t g_table[ST_MAX][EV_MAX] = {
    [ST_INIT]  = { [EV_A] = h_init_a,  [EV_B] = h_init_b },   /* INIT 状态转换 */
    [ST_READY] = { [EV_A] = h_ready_a, [EV_B] = h_ready_b },  /* READY 状态转换 */
    [ST_RUN]   = { [EV_A] = h_run_a,   [EV_B] = h_run_b },    /* RUN 状态转换 */
};

/* 按状态与事件查表取 handler */
handler_t r13_lookup(State s, Event e)
{
    /* 查状态转换表 */
    return g_table[s][e];
}
