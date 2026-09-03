# r08-missing-lock-increment 用例说明（三段式）

## 1. 真实仓形态
全局计数器被多个工作线程并发 `++`，未加锁/原子——经典数据竞争，计数器丢失更新。
主线程则等工作线程全部结束后读总数做汇总（fork/join 形态）。

## 2. 真缺陷在哪
- **真缺陷**：`r08_inc` 的 `g_counter++` 非原子，多个工作线程并发执行时读写
  竞争（lost update），真实 cwe-362。
- **安全点**：`r08_read` 的使用契约是「仅在全部工作线程 join 之后由主线程
  调用」。线程退出与 join 返回之间建立 happens-before，工作线程此前的所有写
  对 join 后的读可见，且此时已无任何并发写——读 `g_counter` 无数据竞争。
  该依据已写入源码使用形态注释与 golden note，与 must_find 的多线程前提
  不自相矛盾：竞争发生在工作线程并发执行期间（inc），join 之后的读（read）
  不在竞争窗口内。

## 3. 各工具误判方式
- CSA：能识别数据竞争（强项，但需 -analyzer-config 多线程或 thread-safety）。
- CppCheck / clang-tidy：可能需 -Wthread-safety 或专用 checker。
- cooddy：若做并发分析，应能识别——对照。
