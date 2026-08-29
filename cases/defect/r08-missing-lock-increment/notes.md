# r08-missing-lock-increment 用例说明（三段式）

## 1. 真实仓形态
全局计数器被多线程并发 `++`，未加锁/原子——经典数据竞争，计数器丢失更新。

## 2. 真缺陷在哪
- **真缺陷**：`r08_inc` 的 `g_counter++` 非原子，并发读写竞争（cwe-362）。
- **安全点**：`r08_read` 单线程读，无竞争。

## 3. 各工具误判方式
- CSA：能识别数据竞争（强项，但需 -analyzer-config 多线程或 thread-safety）。
- CppCheck / clang-tidy：可能需 -Wthread-safety 或专用 checker。
- cooddy：若做并发分析，应能识别——对照。
