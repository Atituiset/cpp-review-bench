# c16-enum-closed-switch 用例说明（三段式）

## 1. 真实仓形态（这个 case 从哪来）
事件分发常对**闭合枚举**写 switch 全覆盖且**故意不写 default**——目的是让编译器在
未来新增枚举值却漏处理时告警（-Wswitch）。工具按「switch 无 default 可能漏处理」
误报逻辑缺陷，但枚举闭合时无 default 是设计。

## 2. 为什么契约安全 / 哪里是真缺陷
- **安全点**：`Evt` 仅 A/B/C，switch 全覆盖无 default——契约安全，无 default 是设计。
- **真缺陷（混入）**：`EVT_C` 分支 `payload[n]` 索引，n 外部传入未校验，n>=3 越界
  读（cwe-125）。这是真实缺陷，与「无 default」无关。

## 3. 各工具可能误判方式
- CSA / CppCheck / clang-tidy：可能把「无 default」误报为漏处理（cwe-unknown/logic）；
  对 payload[n] 越界可能因 n 来源不明漏报。
- cooddy：若理解枚举闭合 + 识别 n 未约束，应能不误报无 default 且查到越界——对照。
