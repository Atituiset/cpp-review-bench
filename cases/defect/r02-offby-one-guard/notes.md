# r02-offby-one-guard 用例说明（三段式）

## 1. 真实仓形态
报文解析常写 `for (i=0; i<len; i++)` 拷贝，但守卫边界少算一字节（写 `i < len+1` 或
`memcpy(dst, src, len+1)`）是高频 off-by-one 缺陷。

## 2. 真缺陷在哪
- **真缺陷**：`r02_copy` 的 `i < len + 1u` 多拷一字节，`dst[len]` 越界写（cwe-787）。
- **安全点**：`r02_copy_ok` 的 `i < len` 正确受界，非缺陷。

## 3. 各工具误判方式
- CSA / CppCheck / clang-tidy：应能查到 off-by-one（这是它们的强项），用于对照基线。
- cooddy：符号执行应能精确识别多一字节越界——对照精度。
