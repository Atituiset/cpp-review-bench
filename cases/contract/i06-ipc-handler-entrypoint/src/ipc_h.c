/*
 * i06-ipc-handler-entrypoint：IPC 注册 handler 本地零调用（死代码陷阱）
 *
 * 真实形态：handler 通过 REGISTER_HANDLER 注册到框架，框架在 IPC 消息到达时
 * 回调。本地代码**零直接调用** handler——静态分析易误判 handler 为「死代码/
 * 未使用函数」（cwe-unknown）。但 handler 是框架入口，非死代码，安全。
 *
 * 混入真实缺陷：handler 内部对 payload 索引未约束（cwe-125 越界读）。
 */
#include <stdint.h>
#include <stddef.h>

typedef void (*ipc_handler_t)(const uint8_t *, uint8_t);

/* 框架注册表（简化）：handler 注册进框架，由框架在消息到达时回调 */
static ipc_handler_t g_registry[4];

void REGISTER_HANDLER(int slot, ipc_handler_t h)
{
    if (slot >= 0 && slot < 4) {
        g_registry[slot] = h;   /* 注册，框架持有引用 */
    }
}

/* handler：本地零直接调用，由框架回调（must_not_find 守护：非死代码） */
static void on_msg(const uint8_t *payload, uint8_t n)
{
    /* 锚点（must_find）：payload[n] 索引，n 未校验，n>=某界时越界读 */
    uint8_t tag = payload[n];   /* 真实 cwe-125 越界读缺陷 */
    (void)tag;
    /* 锚点（must_not_find）：on_msg 被框架回调，非死代码——见 notes */
    (void)payload;
}

/* 初始化：仅注册，不调用 handler（本地零调用） */
void ipc_setup(void)
{
    REGISTER_HANDLER(0, on_msg);   /* handler 注册入框架，由框架调用 */
}
