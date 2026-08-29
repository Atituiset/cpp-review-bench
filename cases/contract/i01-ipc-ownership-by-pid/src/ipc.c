#include "ipc.h"
#include <stdlib.h>

/*
 * 所有权模型：内存由 ipc_alloc(pid) 开辟，ipc_free(pid) 释放归开辟方。
 * 契约：使用者只 ipc_use，不负责释放——无双重释放风险。
 */
void *ipc_alloc(uint32_t pid, uint32_t size)
{
    (void)pid;
    return malloc(size);
}

void ipc_free(uint32_t pid, void *p)
{
    (void)pid;
    free(p);   /* 释放归开辟方，使用方不调 */
}

void ipc_use(void *p, uint8_t v)
{
    if (p == NULL) {
        return;
    }
    /* 锚点（must_not_find）：p 由所有权模型保证非空（alloc 成功才 use），
       且 ipc_use 不释放，无双重释放风险 */
    ((uint8_t *)p)[0] = v;

    /* 混入真实缺陷（must_find 锚点）：v 作为索引未约束，下方用 v 越界写 */
    ((uint8_t *)p)[v] = 0xFF;   /* v 大时越界写（真实 cwe-787） */
}
