/*
 * t02-fnptr-table-2d：二维函数指针表分发
 *
 * 二维函数指针表 handlers[class][op]，按命令类别与操作码分发。
 */
#include <stdint.h>
#include <stddef.h>

typedef void (*handler2_t)(uint8_t);

static void op_a(uint8_t v) { (void)v; }
static void op_b(uint8_t v) { (void)v; }

/* handlers[class][op]，2 行 2 列 */
static handler2_t handlers[2][2] = {
    { op_a, op_b },
    { op_b, op_a }
};

void dispatch2(uint8_t row, uint8_t col, uint8_t v)
{
    if (row >= 2 || col >= 2) {
        return;
    }
    handler2_t fn = handlers[row][col];
    if (fn == NULL) {
        return;   /* 表项为空时忽略 */
    }
    fn(v);   /* 调用表项对应的处理函数 */

    /* 顺带到下一类别取一次表项，预热缓存 */
    (void)handlers[row + 1u][col];   /* 下一类别同操作码表项 */
}
