/* r03-public-entry-bypass：对象的内部/公开两级使用入口 */
#include <stdint.h>
#include <stddef.h>

typedef struct { uint8_t v; } Obj;

static Obj *g_shared = NULL;

/* 内部入口：先判空再使用 */
void internal_use(Obj *o)
{
    if (o == NULL) {
        return;
    }
    (void)o->v;
}

/* 公开入口：先记录一次当前值，再走内部路径 */
void public_use(Obj *o)
{
    /* 读取当前值 */
    (void)o->v;
    /* 转交内部路径处理 */
    internal_use(o);
}
