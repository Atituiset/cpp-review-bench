/*
 * c21-startup-global-init：全局量启动期 malloc，业务期使用
 *
 * 真实形态：协议栈常把大缓冲作为全局量，在 startup 阶段一次性分配，
 * 业务路径直接使用（契约保证 init 先于 use）。传统 SA 看不到 init 调用，
 * 容易把业务期的使用报成「可能空指针/未初始化」（cwe-476 误报）。
 *
 * 混入真实缺陷：write_at 里 offset 回绕导致越界写（cwe-190/cwe-787）。
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define GBUF_N 256

/* 启动期分配，业务期复用（契约：init 必须先于 use） */
static uint8_t *g_buf = NULL;
static uint8_t  g_len = 0;

int gbuf_init(void)
{
    g_buf = (uint8_t *)malloc(GBUF_N);
    if (g_buf == NULL) {
        return -1;
    }
    g_len = GBUF_N;
    memset(g_buf, 0, GBUF_N);
    return 0;
}

/* 业务期使用：契约保证 gbuf_init 已调用，g_buf 非空 */
void gbuf_write_at(uint8_t off, uint8_t val)
{
    /* 锚点（must_not_find）：g_buf[off] 使用，g_buf 非空由 init 契约保证 */
    g_buf[off] = val;

    /* 混入真实缺陷（must_find 锚点）：off 为 uint8_t，传入 255 时 +1 回绕为 0，
       下方按回绕后的长度索引越界 */
    uint8_t end = off + 1u;          /* off=255 → end=0，边界失真 */
    if (end < g_len) {
        g_buf[end] = 0xFF;
    }
}
