#include "util.h"

/*
 * validate：报文入口校验，判空 + len 边界检查。
 * 返回 0 表示校验通过，负值为对应错误码。
 */
int validate(Pkt *p)
{
    if (p == NULL) {
        return -1;
    }
    if (p->len > 128) {
        return -2;
    }
    return 0;
}
