/*
 * c03-assert-guard：自定义断言宏守护的解引用
 *
 * 真实形态：嵌入式协议解析里常见「自研断言宏」在解引用入口守护，
 * 评审者/工具看到解引用容易误报 cwe-476（空指针解引用），
 * 但断言已保证运行期非空——属契约安全。
 *
 * 同时混入一个真实的整数回绕缺陷（must_find），让对照有意义：
 * msg->len 为 uint8_t，累加 header+body 时回绕。
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

/* 入口：断言守护后的安全解引用（must_not_find 锚点） */
int msg_dispatch(Msg *msg)
{
    MSG_REQUIRE(msg != NULL, -1);          /* 断言守护 */
    MSG_REQUIRE(msg->len <= 64, -2);

    /* 断言已保证 msg 非空，下方解引用恒安全 */
    uint8_t head = msg->buf[0];            /* 锚点：msg->buf[0]，被断言守护 */
    (void)head;

    /* 混入真实缺陷：total 回绕（must_find 锚点） */
    uint8_t total = msg->len + 1u;         /* +1 头长度，len=255 时回绕为 0 */
    if (total > 0) {
        msg->buf[total] = 0xAA;            /* len=255 时 total=0，写入 buf[0] 越界？否——边界在 total<=64 已守，但回绕使 total 计算失真 */
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
