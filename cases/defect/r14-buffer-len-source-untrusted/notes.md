# r14-buffer-len-source-untrusted 用例说明（三段式）

## 1. 真实仓形态
网络接收函数把报文头里的 len 字段直接 memcpy 进定长栈缓冲，未校验长度——外部长度
不可信导致的堆/栈越界写。

## 2. 真缺陷在哪
- **真缺陷**：`r14_ingest` 的 `memcpy(buf, payload, hdr_len)` 中 hdr_len 来自外部未
  约束，hdr_len>R14_BUF 时越界写（cwe-787）。
- **安全点**：`r14_ingest_ok` 的 `if (hdr_len > R14_BUF) return;` 受界，安全。

## 3. 各工具误判方式
- CSA / CppCheck / clang-tidy：应能查到无界 memcpy（强项），基线对照。
- cooddy：符号执行识别未约束外部 len——对照。
