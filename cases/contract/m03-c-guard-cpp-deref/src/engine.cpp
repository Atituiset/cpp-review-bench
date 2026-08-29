// m03-c-guard-cpp-deref：C 入口判空 → C++ 引擎解引用（跨语言）
//
// 真实形态：C 侧入口判空后，调用 C++ 引擎方法解引用（跨语言边界）。
// 契约：C 侧已判空，C++ 引擎内部假设非空——跨语言守护，安全。
//
// 混入真实缺陷：C++ 引擎内部对 buf 索引未约束（cwe-125 越界读）。

#include <cstdint>

struct Engine {
    const uint8_t *buf;
    uint8_t len;
    uint8_t process(uint8_t idx) const;
};

uint8_t Engine::process(uint8_t idx) const
{
    // 锚点（must_find）：buf[idx] 索引，idx 未校验，idx>=len 时越界读
    return buf[idx];   // 真实 cwe-125 越界读缺陷
}
