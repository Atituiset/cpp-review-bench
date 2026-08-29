# r04-oob-write-stack 用例说明（三段式）

## 1. 真实仓形态
网络接收函数把外部长度字段直接 `memcpy` 进定长栈缓冲，未校验长度——经典栈溢出。

## 2. 真缺陷在哪
- **真缺陷**：`r04_recv` 的 `memcpy(buf, payload, len)` 中 len 来自外部未校验，
  len>STK_BUF 时栈缓冲越界写（cwe-787）。
- **安全点**：`r04_recv_ok` 的 `if (len > STK_BUF) return;` 受界，安全。

## 3. 各工具误判方式
- CSA / CppCheck / clang-tidy：应能查到无界 memcpy（强项），基线对照。
- cooddy：符号执行应能识别未约束 len 的越界——对照。
