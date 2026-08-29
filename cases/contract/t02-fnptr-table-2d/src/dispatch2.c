/*
 * t02-fnptr-table-2d：二维数组表，入口判空
 *
 * 真实形态：二维函数指针表 handlers[class][op]，分发前判空。工具可能误报
 * cwe-476（函数指针为空）——但入口判空守护，安全。
 *
 * 混入真实缺陷：二维索引 row/col 未约束导致 handlers[row][col] 越界（cwe-125）。
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
        return;   /* 判空（must_not_find 守护） */
    }
    fn(v);   /* 锚点（must_not_find）：fn 已判空，调用安全 */

    /* 混入真实缺陷（must_find 锚点）：另一条路径用 row+1 索引二维表，
       row=1 时 handlers[row+1][col] 越界读 handlers[2][col] */
    (void)handlers[row + 1u][col];   /* row=1 时越界 */
}
