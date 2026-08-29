# r03-public-entry-bypass 用例说明（三段式）

## 1. 真实仓形态
模块有内部函数（判空）和公开 API（未判空）。调用方绕过内部函数直接调公开 API 传
NULL，触发空指针解引用——经典「判空覆盖不全」缺陷。

## 2. 真缺陷在哪
- **真缺陷**：`public_use` 未判空直接 `o->v`，外部可传 NULL 绕过 `internal_use` 的
  判空路径（cwe-476）。
- **安全点**：`internal_use` 路径判空，安全。

## 3. 各工具误判方式
- CSA / CppCheck / clang-tidy：应能查到 public_use 的 cwe-476（单 TU 可见）。
- cooddy：跨过程分析应能识别绕过路径——对照。
