# c06-flexible-array-member · 变长结构体按 len 分配访问（FAM 惯用法）

## 真实仓形态

协议栈变长消息体（TLV / 变长 payload）惯用柔性数组成员（FAM）：
`typedef struct { uint16_t len; uint8_t data[]; } Msg;`，分配时
`malloc(sizeof(Msg) + len)` 把 data 区一并分配。访问 data[i] 靠 `i < len` 守卫。
NAS/RRC 的变长 IE、报文封装大量采用此写法（移植自 AetherStack 消息体同形态代码）。

## 为什么契约安全

`msg_new` 分配 `sizeof(Msg) + len`，data 区完整落在缓冲内；`msg_fill` 的循环
`i < n && i < m->len` 双守卫保证下标不越界。分配口径与访问守卫一致，属同仓可核验
契约。单函数/单文件粒度的 SA 看 `m->data[i]` 裸下标可能误报越界读/写，不识别
「FAM + 随结构分配 + 守卫」的惯用法安全。

## 各工具误判方式

- 单函数粒度的 SA：在 `m->data[i] = src[i];` 处报越界（cwe-125/cwe-787），
  不知道 data 区随 Msg 一同分配且循环有 i<m->len 守卫；
- LLM 评审：对 FAM 访问器逐个报「缺少边界检查」；
- 正确行为：确认分配含 data 区 + 访问守卫后豁免；注入 contract.yaml 后仍报记为契约违反。

## 附：混入的 must_find 正例

`msg_copy` 把 src 的 n 字节写入 `dst->data`，但只校验 src 长度、未校验 `dst->len >= n`，
且循环 `i <= n` 越界写 1 字节（cwe-787）——与 FAM 安全写法无关，是独立的真缺陷。
用于探工具「豁免过度」：把整文件当 FAM 安全而放过 msg_copy 的越界写即记 FN。
