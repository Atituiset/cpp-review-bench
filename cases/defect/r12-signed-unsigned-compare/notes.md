# r12-signed-unsigned-compare 用例说明（三段式）

## 1. 真实仓形态
边界检查 `if (len < LIM)` 中 len 为有符号、LIM 无符号，C 的寻常算术转换把 len 升为
无符号，负值变巨大正数绕过检查——经典符号混用缺陷。

## 2. 真缺陷在哪
- **真缺陷**：`r12_use` 的 `len < LIM` 比较时 len<0 转成巨大正数，检查通过，
  `buf[len]` 负索引越界（cwe-190）。
- **安全点**：`r12_use_ok` 先 `len>=0 && len<LIM` 有符号检查在前，安全。

## 3. 各工具误判方式
- CSA / CppCheck / clang-tidy：应能查到符号混用（强项，有 -Wsign-compare），基线对照。
- cooddy：符号执行识别负 len 转换——对照。
