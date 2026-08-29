// m01-extern-c-entrypoint：extern "C" 导出被 C 侧调用（死代码陷阱）
//
// 真实形态：C++ 模块用 extern "C" 导出 entry 符号，供 C 侧链接调用。
// C++ TU 内 entry 本地零直接调用——静态分析（尤其单 TU）易误报「未使用/死代码」。
// 但 entry 是 C 侧链接入口，非死代码，安全。
//
// 混入真实缺陷：entry 内对 buf 索引未约束（cwe-125 越界读）。

#include <cstdint>

extern "C" void m01_entry(const uint8_t *buf, uint8_t n)
{
    // 锚点（must_find）：buf[n] 索引，n 未校验，n 超界时越界读
    uint8_t t = buf[n];   // 真实 cwe-125 越界读缺陷
    (void)t;
    // 锚点（must_not_find）：m01_entry 由 C 侧链接调用，非死代码——见 notes
    (void)buf;
}
