#include "ipc.h"
#include <stdlib.h>

/*
 * 所有权模型：内存由 ipc_alloc(pid) 开辟，ipc_free(pid) 释放归开辟方。
 * 使用者只调 ipc_use，不负责释放。
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
    /* 写首字节作为就绪标志 */
    ((uint8_t *)p)[0] = v;

    /* 按值初始化对应槽位 */
    ((uint8_t *)p)[v] = 0xFF;   /* 槽位置位 */
}
