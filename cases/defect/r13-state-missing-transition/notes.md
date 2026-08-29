# r13-state-missing-transition 用例说明（三段式）

## 1. 真实仓形态
状态机用二维 handler 表，事件枚举 EV_A/B/C 合法，但表初始化漏填 EV_C 槽位——
合法事件无 handler，运行时取到 NULL 调用崩溃。逻辑缺陷（logic）。

## 2. 真缺陷在哪
- **真缺陷**：`r13_lookup` 返回 `g_table[s][e]`，e=EV_C 时槽位未初始化（NULL），
  调用方取到 NULL handler（logic 缺陷）。
- **安全点**：EV_A/EV_B 已显式置 0（即便 0 是占位），枚举与表一致点非缺陷。

## 3. 各工具误判方式
- CSA / clang-tidy：可能查到「返回未初始化值」或「表缺槽位」。
- cooddy：若做枚举-表一致性分析，应能识别漏填 EV_C——对照。
- 这是 Agent Viewer 要学的「枚举全集 vs 分发表一致性」审查。
