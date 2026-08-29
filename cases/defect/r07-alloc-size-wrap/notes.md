# r07-alloc-size-wrap 用例说明（三段式）

## 1. 真实仓形态
`malloc(n * size)` 中 n/size 为窄类型时乘法先回绕，分配过小，后续按原大小写入越界——
经典整数回绕 + 堆溢出。

## 2. 真缺陷在哪
- **真缺陷**：`r07_alloc` 的 `(size_t)(n * size)` 先 uint32 乘回绕，malloc 不足，
  后续写入越界（cwe-190+787）。
- **安全点**：`r07_alloc_ok` 的 `(size_t)n*(size_t)size` size_t 域乘法不回绕，受界。

## 3. 各工具误判方式
- CSA / CppCheck：对乘法回绕敏感度高（强项），基线对照。
- cooddy：符号执行应能精确识别回绕——对照。
