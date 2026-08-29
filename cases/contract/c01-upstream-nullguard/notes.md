# c01-upstream-nullguard · 入口判空后多层转发解引用

## 真实仓形态

电信/协议栈里典型的「入口防御」风格：对外 API（`guti_encode_size`）对可空入参判空一次，
内部一串 static 小函数逐层转发后直接解引用，不再每层重复判空。NAS/AS 层编码器里
IE 尺寸计算、payload 偏移推算大量采用这种写法（移植自 AetherStack NAS 模块同形态代码）。

## 为什么契约安全

`ie_field_len` 是文件内静态函数，全模块唯一调用链为
`guti_encode_size → guti_body_size → guti_payload_size → ie_wire_size → ie_field_len`，
链头对 `guti == NULL` 返回 -1 且不进入转发。静态可达性上 `ie_field_len` 的 `ie` 恒非空，
解引用安全。判空逻辑与解引用同文件、同翻一屏可见，属「局部可核验」的契约。

## 各工具误判方式

- 过程内/过程间分析不足的 SA：在 `ie_field_len` 内看到裸解引用即报 cwe-476，不回溯唯一调用点的判空守卫；
- LLM 评审：逐函数审读时对中间层（`ie_wire_size`/`guti_payload_size`）报「缺少空指针检查」；
- 正确行为：识别入口判空 + 静态函数唯一调用链后豁免；注入 contract.yaml 后仍报则记为契约违反。

## 附：混入的 must_find 正例

`guti_group_size` 用 `uint8_t` 累加整组 IE 尺寸（cwe-190）：IE 稍多即回绕，
调用方按回绕值分配缓冲区会越界写。用于探工具「豁免过度」——把整文件当契约安全放过即记 FN。
