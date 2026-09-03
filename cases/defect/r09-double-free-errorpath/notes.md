# r09-double-free-errorpath 用例说明（三段式）

## 1. 真实仓形态
错误路径释放资源后**漏写 return**，落到函数末尾再次释放同一指针——经典双重释放。

## 2. 真缺陷在哪
- **真缺陷**：`r09_open` 错误路径 `free(c->buf)` 后未置 NULL 也未 return，落到
  末尾再次 `free(c->buf)`；sz > 65535 时（如 sz=70000）同一指针被释放两次，
  双重释放（cwe-415）。
- **安全点**：`r09_open_ok` 错误路径释放后 `return -2;`，每个分支只释放一次，
  无重复释放。

## 3. 各工具误判方式
- CSA / CppCheck / clang-tidy：应能查到双重释放（强项），基线对照。
- cooddy：符号执行识别错误路径未 return——对照。
