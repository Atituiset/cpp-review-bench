# c01b-crossfile-nullguard 用例说明（三段式）

## 1. 真实仓形态（这个 case 从哪来）
大型 C 项目里判空与解引用常分布在**两个 .c 文件**：`util.c` 提供 `validate()`
（判空 + 边界），`caller.c` 的 `consume()` 先调 `validate` 再解引用。传统 SA 默认
单 TU 分析，看不到跨文件契约，容易把 `consume` 里的 `p->tag` 解引用报成 cwe-476。

## 2. 为什么契约安全 / 哪里是真缺陷
- **安全点**：`consume` 先 `if (validate(p) != 0) return;`，`validate` 已判空且
  校验 `len<=128`，后续 `p->tag` 解引用运行期恒安全——跨文件契约安全。
- **真缺陷（混入）**：`(uint8_t)p->len + 1u`——validate 已限 `len<=128`，强转后
  `sum=len+1<=129`，回绕不可能；但 len>=127 时 sum>=128，`payload[sum]` 越过
  `payload[128]` 末元素越界写（cwe-787）。守卫截断后的 off-by-one。

## 3. 各工具可能误判方式
- CSA（单 TU）：看不到 validate 判空，可能误报 cwe-476；CTU 模式应能跨文件看到
  validate 的契约，不误报——用于对照单 TU vs CTU 的差异。
- CppCheck / clang-tidy：通常单文件，可能误报 cwe-476。
- cooddy：若函数摘要/跨过程分析识别 validate 守护，不误报；用于对照。
