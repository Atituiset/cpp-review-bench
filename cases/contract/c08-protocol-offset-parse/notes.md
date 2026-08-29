# c08-protocol-offset-parse · 协议解析步进偏移，入口统一校验

## 真实仓形态

协议栈解析层的经典分工：一个集中的结构校验函数（`u2u_frame_valid`）+ 一组不做边界检查的
字段访问器（`u2u_fields.c`）。校验与提取跨两个文件，靠「入口先校验」的模块契约衔接。
变长字段（IMSI 两段）步进偏移的写法在 NAS/RR/短信编解码里随处可见
（移植自 AetherStack U2U 模块同形态代码）。

## 为什么契约安全

`u2u_frame_valid` 逐步确认 `size >= 12`、`size >= 12 + src_len`、`size >= 12 + src_len + dst_len`，
通过后访问器读取的所有偏移（`frame[10]`、`frame[11 + src_len]`、payload 起始）均已被证明在界内。
业务入口 `u2u_payload_offset_checked` 严格先校验后提取，整条路径安全。
契约的关键在于「校验逻辑与访问器偏移口径一致」——同一组常量与字段顺序，同仓可核验。

## 各工具误判方式

- 单函数/单文件粒度的 SA：在 `u2u_fields.c` 内看到 `frame[11 + src_len_at(frame)]` 直接报越界读，
  不知道（或不信任）跨文件的入口校验；
- LLM 评审：对无检查的访问器逐个报「缺少边界检查」，或反过来把 `u2u_payload_type_peek` 也当成
  「已有 size 检查」而放过；
- 正确行为：沿调用点确认校验先于提取后豁免访问器；注入 contract.yaml 后仍报记为契约违反。

## 附：混入的 must_find 正例

`u2u_payload_type_peek`（告警统计旁路）只查 `size >= 12` 就直调无检查访问器，
IMSI 长度字段取自报文可达 255，payload 偏移可严重越界——与安全路径只差一步 `u2u_frame_valid`。
同一 scenario（cwe-125）一例豁免一例真缺陷，专门探工具能否按「校验是否覆盖」做细粒度区分，
而非对全文件一刀切豁免或一刀切报警。
