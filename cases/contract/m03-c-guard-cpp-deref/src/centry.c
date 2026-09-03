#include <stdint.h>
#include <stddef.h>

/* C 侧入口：判空后调用 C++ 引擎（跨语言守护） */
struct Engine;
#ifdef __cplusplus
extern "C" {
#endif
uint8_t m03_engine_process(struct Engine *e, uint8_t idx);
#ifdef __cplusplus
}
#endif

void c_entry(struct Engine *e, uint8_t idx)
{
    if (e == NULL) {
        return;   /* C 侧判空守护 */
    }
    // 判空后交给 C++ 引擎处理
    (void)m03_engine_process(e, idx);
}
