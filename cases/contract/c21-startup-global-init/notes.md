# c21-startup-global-init 用例说明（三段式）

## 1. 真实仓形态（这个 case 从哪来）
协议栈/驱动常把大缓冲声明为**全局量**，在 startup（或模块 init）阶段一次性
`malloc`，业务路径直接解引用。传统 SA 单 TU 视野看不到 init 调用，容易把业务期的
使用报成 cwe-476（可能空指针/未初始化）。

## 2. 为什么契约安全 / 哪里是真缺陷
- **安全点**：`gbuf_init` 启动期分配 `g_buf`，`gbuf_write_at` 业务期 `g_buf[off]`
  使用；契约保证 init 先于 use，运行期非空——安全。
- **真缺陷（混入）**：`uint8_t end = off + 1u`——off 为 uint8_t，off=255 时 +1
  回绕为 0，边界检查 `end < g_len` 失真，下游 `g_buf[end]` 索引错位（cwe-190）。

## 3. 各工具可能误判方式
- CSA / CppCheck / clang-tidy：可能把 `g_buf[off]` 报成 cwe-476 误报（看不到 init
  契约）；也可能因 uint8_t 回绕不敏感漏掉 must_find。
- cooddy：若跨过程生命周期分析识别 init 守护，不误报；用于对照。
