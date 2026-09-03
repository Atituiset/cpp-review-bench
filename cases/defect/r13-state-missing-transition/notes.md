# r13-state-missing-transition 用例说明（三段式）

## 1. 真实仓形态
状态机用二维 handler 表，事件枚举 EV_A/B/C 合法，但表初始化漏填 EV_C 槽位——
合法事件无 handler，运行时查表取到 NULL，调用方解引用即崩溃。逻辑缺陷（logic）。

## 2. 真缺陷在哪
- **真缺陷**：`r13_lookup` 返回 `g_table[s][e]`；EV_A/EV_B 槽位均填有真实
  handler（h_init_a 等），唯独 EV_C 一列三个状态全部未填（隐式零初始化即
  NULL）。e=EV_C 时返回 NULL handler，调用方 `handler()` 即空指针调用
  （logic 缺陷，运行时可复现为段错误）。
- **安全点**：EV_A/EV_B 各槽位均为真实填充的 handler（如 ST_INIT 行的
  h_init_a/h_init_b），枚举值与表项一致——对这些槽位报「缺失/未初始化」
  属误报。正负例由此可区分：EV_A/EV_B 查表返回非 NULL 且可正常调用，
  EV_C 查表返回 NULL。

## 3. 各工具误判方式
- CSA / clang-tidy：可能查到「返回未初始化值」或「表缺槽位」。
- cooddy：若做枚举-表一致性分析，应能识别漏填 EV_C——对照。
- 这是 Agent Viewer 要学的「枚举全集 vs 分发表一致性」审查。
