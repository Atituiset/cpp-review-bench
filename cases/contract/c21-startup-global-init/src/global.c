/*
 * c21-startup-global-init：全局量启动期 malloc，业务期使用
 *
 * 协议栈常把大缓冲作为全局量，在 startup 阶段一次性分配，
 * 业务路径直接使用（约定 init 先于 use）。
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

/* 业务期写入：按约定 gbuf_init 已在启动期完成 */
void gbuf_write_at(uint8_t off, uint8_t val)
{
    g_buf[off] = val;

    /* 在写入位置之后再补一个结束标记 */
    uint8_t end = off + 1u;          /* 结束标记位置 */
    if (end < g_len) {
        g_buf[end] = 0xFF;
    }
}
