/*
 * c03-assert-guard：自定义断言宏守护的协议解析入口
 *
 * 嵌入式协议解析里常见「自研断言宏」在入口做前置检查，
 * 条件不满足时直接返回错误码，后续代码按已校验假设编写。
 */
#include <stdint.h>
#include <stdio.h>

/* 自定义断言宏：条件不成立直接返回错误码，不解引用 */
#define MSG_REQUIRE(cond, rc) \
    do { if (!(cond)) { return (rc); } } while (0)

typedef struct {
    uint8_t  len;     /* 载荷长度（不含头） */
    uint8_t  buf[64]; /* 定长缓冲 */
} Msg;

/* 分发入口：处理一条完整消息 */
int msg_dispatch(Msg *msg)
{
    MSG_REQUIRE(msg != NULL, -1);          /* 入参校验 */
    MSG_REQUIRE(msg->len <= 64, -2);

    /* 取消息首字节作为类型标识 */
    uint8_t head = msg->buf[0];            /* 首字节：类型字段 */
    (void)head;

    /* 在载荷末尾追加一个填充字节 */
    uint8_t total = msg->len + 1u;         /* +1 头长度 */
    if (total > 0) {
        msg->buf[total] = 0xAA;            /* 写入填充字节 */
    }
    return 0;
}

/* 辅助：构造一条消息 */
int msg_build(Msg *msg, const uint8_t *src, uint8_t n)
{
    MSG_REQUIRE(msg != NULL, -1);
    MSG_REQUIRE(src != NULL, -1);
    MSG_REQUIRE(n <= 64, -2);
    msg->len = n;
    for (uint8_t i = 0; i < n; i++) {
        msg->buf[i] = src[i];
    }
    return 0;
}
