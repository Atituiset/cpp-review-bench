#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* 变长结构体（柔性数组成员 FAM）按 len 分配访问：
 * Msg 尾随 data[]，分配时 malloc(sizeof(Msg) + len) 已含 data 区；访问
 * data[i] 按 i < m->len 进行。
 * 形态源自协议栈的变长消息体（TLV/变长 payload），FAM 是惯用法。 */

typedef struct {
    uint16_t len;
    uint8_t  data[];   /* flexible array member */
} Msg;

Msg *msg_new(uint16_t len) {
    Msg *m = (Msg *)malloc(sizeof(Msg) + len);
    if (!m) return NULL;
    m->len = len;
    return m;
}

/* 按 n 与 m->len 的较小者填充 data 区。 */
void msg_fill(Msg *m, const uint8_t *src, uint16_t n) {
    for (uint16_t i = 0; i < n && i < m->len; ++i)
        m->data[i] = src[i];
}

/* 把 src 的 n 字节连同结束标志写入 dst->data。 */
void msg_copy(Msg *dst, const uint8_t *src, uint16_t n) {
    for (uint16_t i = 0; i <= n; ++i)
        dst->data[i] = src[i];
}
