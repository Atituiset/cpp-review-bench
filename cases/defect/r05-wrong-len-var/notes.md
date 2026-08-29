# r05-wrong-len-var 用例说明（三段式）

## 1. 真实仓形态
拷贝函数有 `n`（期望长度）和 `m`（另一个长度）两变量，代码误用 `m` 作 memcpy 长度，
m 未约束时越界——长度变量用错缺陷。

## 2. 真缺陷在哪
- **真缺陷**：`r05_copy` 的 `memcpy(buf, payload, m)` 用错变量 m，m 未约束时越界
  （cwe-125）。
- **安全点**：`r05_copy_ok` 用正确变量 n 且受界，安全。

## 3. 各工具误判方式
- CSA / CppCheck / clang-tidy：应能查到 memcpy 长度不匹配/越界（强项），基线对照。
- cooddy：符号执行识别 m 未约束——对照。
