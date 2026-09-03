/*
 * t01-fnptr-table-1d：一维裸函数指针数组分发
 *
 * 协议/命令分发常用一维函数指针表 `handlers[idx]`，分发前判空。
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
        return;   /* 索引上限检查 */
    }
    handler_t fn = handlers[idx];
    if (fn == NULL) {
        return;   /* 表项为空时忽略 */
    }
    fn(v);   /* 调用表项对应的处理函数 */

    /* 把下一个槽位重置为默认处理函数 */
    handlers[idx + 1u] = h_a;   /* 下一槽位置为默认处理 */
}
