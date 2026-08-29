/*
 * t01-fnptr-table-1d：一维裸函数指针数组分发，入口判空
 *
 * 真实形态：协议/命令分发常用一维函数指针表 `handlers[idx]`，分发前判空。
 * 工具可能按「函数指针调用可能为空」误报 cwe-476——但入口已判空，安全。
 *
 * 混入真实缺陷：idx 未约束导致 handlers[idx] 越界读（cwe-125）。
 */
#include <stdint.h>
#include <stddef.h>

typedef void (*handler_t)(uint8_t);

static void h_a(uint8_t v) { (void)v; }
static void h_b(uint8_t v) { (void)v; }
static void h_c(uint8_t v) { (void)v; }

static handler_t handlers[3] = { h_a, h_b, h_c };

/* 分发：入口判空后调用 */
void dispatch(uint8_t idx, uint8_t v)
{
    if (idx >= 3) {
        return;   /* 越界保护 */
    }
    handler_t fn = handlers[idx];
    if (fn == NULL) {
        return;   /* 判空（must_not_find 守护） */
    }
    fn(v);   /* 锚点（must_not_find）：fn 已判空，调用安全 */

    /* 混入真实缺陷（must_find 锚点）：下方再次用 idx 索引但走另一条路径，
       idx 未二次校验（虽上面有保护，这里演示另一入口缺保护） */
    handlers[idx + 1u] = h_a;   /* idx=2 时 idx+1=3 越界写 handlers[3] */
}
