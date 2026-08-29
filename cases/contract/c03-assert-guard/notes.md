# c03-assert-guard 用例说明（三段式）

## 1. 真实仓形态（这个 case 从哪来）
嵌入式/协议栈代码里大量使用**自研断言宏**（如 `MSG_REQUIRE`、`ASSERT_NOT_NULL`）在
函数入口守护指针，而非裸 `if (p == NULL) return`。评审者和传统 SA 看到后续解引用，
容易按「未见显式判空」报 cwe-476 空指针解引用，但宏已在运行期保证非空。

## 2. 为什么契约安全 / 哪里是真缺陷
- **安全点**：`msg_dispatch` 先用 `MSG_REQUIRE(msg != NULL, -1)` 守护，随后
  `msg->buf[0]` 解引用运行期恒非空——契约安全，工具报 cwe-476 即过度敏感。
- **真缺陷（混入）**：`uint8_t total = msg->len + 1u`——len 为 uint8_t，
  当 len=255 时 +1 回绕为 0，total 计算失真。这是真实 cwe-190 整数回绕，
  评审/工具应查到。

## 3. 各工具可能误判方式
- CSA / CppCheck / clang-tidy：可能把 `msg->buf[0]` 解引用报成 cwe-476 误报
  （不理解自研断言宏语义）；也可能因 uint8_t 回绕分析不敏感而漏掉 must_find。
- cooddy（符号执行+约束求解）：理论上能沿断言路径确认 msg 非空（不误报），
  也可能识别回绕——用于对照其相对传统 SA 的精度差异。
- 这正是 Agent Viewer 要学的：「别像传统 SA 那样对宏守护解引用过度敏感」。
