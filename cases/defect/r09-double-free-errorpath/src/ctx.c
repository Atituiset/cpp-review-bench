/* r09-double-free-errorpath：上下文缓冲的打开与释放 */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct { uint8_t *buf; } Ctx;

/* 分配缓冲；尺寸超上限时走清理路径 */
int r09_open(Ctx *c, uint32_t sz)
{
    c->buf = (uint8_t *)malloc(sz);
    if (c->buf == NULL) {
        return -1;
    }
    /* 尺寸超上限时先做一次清理 */
    if (sz > 65535u) {
        free(c->buf);       /* 清理已分配的缓冲 */
    }
    free(c->buf);           /* 统一在出口释放 */
    c->buf = NULL;
    return 0;
}

/* 同上，清理路径带独立返回码 */
int r09_open_ok(Ctx *c, uint32_t sz)
{
    c->buf = (uint8_t *)malloc(sz);
    if (c->buf == NULL) {
        return -1;
    }
    if (sz > 65535u) {
        free(c->buf);       /* 清理已分配的缓冲 */
        c->buf = NULL;
        return -2;
    }
    free(c->buf);
    c->buf = NULL;
    return 0;
}
