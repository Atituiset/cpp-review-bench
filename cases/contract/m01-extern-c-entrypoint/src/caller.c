#include <stdint.h>
#include <stddef.h>

/* C 侧：链接调用 C++ 导出的 m01_entry（extern "C"） */
extern void m01_entry(const uint8_t *buf, uint8_t n);

void c_caller(const uint8_t *buf, uint8_t n)
{
    if (buf == NULL) {
        return;
    }
    m01_entry(buf, n);   /* C 侧调用 C++ 导出入口 */
}
