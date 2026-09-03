/*
 * i06-ipc-handler-entrypoint：IPC 注册 handler，框架回调模式
 *
 * handler 通过 REGISTER_HANDLER 注册到框架，框架在 IPC 消息到达时
 * 回调。本地代码不直接调用 handler，handler 是框架入口。
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

/* handler：注册入框架，由框架在消息到达时回调 */
static void on_msg(const uint8_t *payload, uint8_t n)
{
    /* 取报文第 n 字节作为消息类型 */
    uint8_t tag = payload[n];   /* 消息类型字段 */
    (void)tag;
    (void)payload;
}

/* 初始化：仅注册，不调用 handler（本地零调用） */
void ipc_setup(void)
{
    REGISTER_HANDLER(0, on_msg);   /* handler 注册入框架，由框架调用 */
}
