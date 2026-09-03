// m03-c-guard-cpp-deref：C 入口判空后调用 C++ 引擎
//
// C 侧入口判空后，调用 C++ 引擎方法处理（跨语言边界）。
// 引擎内部按 C 侧已判空的约定实现。

#include <cstdint>

struct Engine {
    const uint8_t *buf;
    uint8_t len;
    uint8_t process(uint8_t idx) const;
};

uint8_t Engine::process(uint8_t idx) const
{
    // 按索引取缓冲中的元素
    return buf[idx];   // 取第 idx 字节
}
