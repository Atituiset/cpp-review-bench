# t01-fnptr-table-1d 用例说明（三段式）

## 1. 真实仓形态（这个 case 从哪来）
命令/协议分发常用**一维函数指针表** `handlers[idx]`，分发前判空再调用。工具按
「函数指针调用可能为空」误报 cwe-476——但入口判空已守护。

## 2. 为什么契约安全 / 哪里是真缺陷
- **安全点**：`dispatch` 先 `if (fn == NULL) return;` 判空再 `fn(v)`——安全。
- **真缺陷（混入）**：`handlers[idx + 1u] = h_a`——idx=2 时 idx+1=3 越界写
  `handlers[3]`（cwe-125）。真实函数指针表越界写缺陷。

## 3. 各工具可能误判方式
- CSA / CppCheck / clang-tidy：可能把 `fn(v)` 误报 cwe-476（未追踪判空）；对
  `handlers[idx+1u]` 越界写可能因 idx 来源/回绕不敏感漏报。
- cooddy：若做数组边界+回绕分析，应能不误报 fn(v) 且查到越界写——对照。
