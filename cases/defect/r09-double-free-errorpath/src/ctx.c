/* r09-double-free-errorpath：错误路径重复释放 cwe-415 */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct { uint8_t *buf; } Ctx;

/* 错误路径里既释放又落入末尾释放，导致双重释放（cwe-415） */
int r09_open(Ctx *c, uint32_t sz)
{
    c->buf = (uint8_t *)malloc(sz);
    if (c->buf == NULL) {
        return -1;
    }
    /* 模拟中途错误：释放后未 return，落到末尾再次释放 */
    if (sz > 65535u) {
        free(c->buf);       /* 锚点（must_find）：错误路径释放，但未 return */
        c->buf = NULL;
    }
    free(c->buf);           /* 锚点（must_find）：末尾再次释放 c->buf → 双重释放（cwe-415） */
    c->buf = NULL;
    return 0;
}

/* 安全点（must_not_find）：错误路径释放后正确 return */
int r09_open_ok(Ctx *c, uint32_t sz)
{
    c->buf = (uint8_t *)malloc(sz);
    if (c->buf == NULL) {
        return -1;
    }
    if (sz > 65535u) {
        free(c->buf);       /* 锚点（must_not_find）：释放后 return，无双重释放 */
        c->buf = NULL;
        return -2;
    }
    free(c->buf);
    c->buf = NULL;
    return 0;
}
