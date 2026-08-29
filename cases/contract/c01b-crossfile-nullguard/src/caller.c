#include "util.h"

/*
 * consume：调用方（本文件）先 validate 再解引用——跨文件契约安全。
 * 工具若按「单文件视野」看不到 validate 的判空，可能误报 cwe-476。
 */
void consume(Pkt *p)
{
    if (validate(p) != 0) {
        return;
    }
    /* 跨文件契约：validate 已保证 p 非空且 len<=128，下方解引用安全 */
    uint8_t t = p->tag;                 /* 锚点：p->tag，被跨文件 validate 守护 */
    (void)t;

    /* 混入真实缺陷：sum 回绕（must_find 锚点） */
    uint8_t sum = (uint8_t)p->len + 1u; /* len=255 时回绕为 0，索引错位 */
    p->payload[sum] = 0x00;             /* 边界由 len<=128 守，但 sum 计算已失真 */
}
