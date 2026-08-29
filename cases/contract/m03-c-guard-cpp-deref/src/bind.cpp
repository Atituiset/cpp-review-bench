#include <cstdint>
#include "engine.cpp"   // 同 TU 内提供 Engine::process 实现

extern "C" uint8_t m03_engine_process(struct Engine *e, uint8_t idx)
{
    if (e == NULL) {
        return 0;
    }
    return e->process(idx);
}
