/* r03-public-entry-bypass：判空仅覆盖单一路径，公开入口可绕过 cwe-476 */
#include <stdint.h>
#include <stddef.h>

typedef struct { uint8_t v; } Obj;

static Obj *g_shared = NULL;

/* 内部入口：判空 */
void internal_use(Obj *o)
{
    if (o == NULL) {
        return;
    }
    (void)o->v;
}

/* 公开入口：未判空，可绕过 internal_use 的判空直接解引用 */
void public_use(Obj *o)
{
    /* 锚点（must_find）：public_use 未判空直接解引用 o->v，外部可传 NULL 绕过
       internal_use 的判空，导致 cwe-476 空指针解引用 */
    (void)o->v;
    /* 锚点（must_not_find）：internal_use 路径已判空，安全 */
    internal_use(o);
}
