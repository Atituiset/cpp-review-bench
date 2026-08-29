#include "util.h"

/*
 * validate：契约入口，判空 + len 边界检查。
 * 契约保证：通过 validate 的 Pkt* 后续解引用恒安全。
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
