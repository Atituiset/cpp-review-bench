# t02-fnptr-table-2d 用例说明（三段式）

## 1. 真实仓形态（这个 case 从哪来）
复杂分发用**二维函数指针表** `handlers[class][op]`（如 class=协议族、op=操作）。
入口判空后调用。工具按「函数指针可能为空」误报 cwe-476——但入口判空守护。

## 2. 为什么契约安全 / 哪里是真缺陷
- **安全点**：`dispatch2` 先 `if (fn == NULL) return;` 判空再 `fn(v)`——安全。
- **真缺陷（混入）**：`handlers[row + 1u][col]`——row=1 时越界读 `handlers[2][col]`
  （cwe-125）。真实二维表越界读缺陷。

## 3. 各工具可能误判方式
- CSA / CppCheck / clang-tidy：可能误报 `fn(v)` 为 cwe-476（未追踪判空）；对
  二维越界可能因回绕/符号不敏感漏报。
- cooddy：若做二维数组边界分析，应能不误报 fn(v) 且查到越界——对照。
