#include <stddef.h>
#include <stdint.h>

/* 错误码集中处理契约（centralized_error_handling）：
 * 管线各 stage 返回 int 错误码（0 成功，<0 失败），但调度层 run_pipeline
 * 不逐个判返回值，而是用最后一次调用的结果 last 统一上报。中间层
 * 「不检查返回值」是约定行为，不是缺陷（见 contract.yaml）。
 * 形态源自电信协议栈的「上行管线」：auth→decrypt→encode 多段处理，
 * 错误统一在管线末尾映射成业务错误码。 */

typedef struct {
    int *buf;
    int n;
} Ctx;

/* 各 stage：返回错误码，失败即返回负；本层不做业务补偿 */
static int stage_auth(Ctx *c) {
    if (!c) return -1;
    return 0;
}

static int stage_decrypt(Ctx *c) {
    if (c->n <= 0) return -1;   /* n 越界由上层契约保证不会发生 */
    return 0;
}

/* 真缺陷点（混入选例，探豁免过度）：按 c->n 写入 c->buf 但循环条件
 * i<=c->n 越界 1 写，且分配 cap 假设按 n 但无上限校验。 */
static int stage_encode(Ctx *c) {
    for (int i = 0; i <= c->n; ++i)
        c->buf[i] = i;
    return 0;
}

/* 调度层：集中错误出口，中间 stage 调用不判返回值。 */
int run_pipeline(Ctx *c) {
    int last = 0;
    last = stage_auth(c);   /* 不判：错误经 last 统一返回 */
    last = stage_decrypt(c);
    last = stage_encode(c);
    return last;            /* 唯一错误出口 */
}
