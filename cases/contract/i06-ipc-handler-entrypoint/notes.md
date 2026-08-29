# i06-ipc-handler-entrypoint 用例说明（三段式）

## 1. 真实仓形态（这个 case 从哪来）
IPC/插件框架里 handler 经 `REGISTER_HANDLER` 注册，框架在消息到达时回调。**本地代码
零直接调用** handler。静态分析易误判 handler 为「死代码/未使用函数」——但它是框架
入口，非死代码。

## 2. 为什么契约安全 / 哪里是真缺陷
- **安全点**：`on_msg` 注册入框架，由框架回调——非死代码，安全。
- **真缺陷（混入）**：`on_msg` 内 `payload[n]` 索引，n 来自 IPC 消息未校验，n 超界
  越界读（cwe-125）。真实缺陷。

## 3. 各工具可能误判方式
- CSA / CppCheck / clang-tidy：可能把 `on_msg` 误报为「未使用函数/死代码」；
  对 `payload[n]` 越界可能因 n 来源（IPC）不敏感漏报。
- cooddy：若理解框架回调注册模式，应能不误报死代码且查到越界——对照。
