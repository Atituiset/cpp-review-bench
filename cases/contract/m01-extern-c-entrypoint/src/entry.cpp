// m01-extern-c-entrypoint：extern "C" 导出给 C 侧调用
//
// C++ 模块用 extern "C" 导出 entry 符号，供 C 侧链接调用。
// C++ TU 内 entry 无本地调用点，调用来自 C 侧链接单元。

#include <cstdint>

extern "C" void m01_entry(const uint8_t *buf, uint8_t n)
{
    // 取报文第 n 字节作为类型字段
    uint8_t t = buf[n];   // 消息类型字段
    (void)t;
    (void)buf;
}
