#ifndef I01_IPC_H
#define I01_IPC_H

#include <stdint.h>
#include <stddef.h>

/* IPC 所有权模型：按 pid 申请，释放归开辟方（契约） */
void *ipc_alloc(uint32_t pid, uint32_t size);
void  ipc_free(uint32_t pid, void *p);
void  ipc_use(void *p, uint8_t v);

#endif /* I01_IPC_H */
