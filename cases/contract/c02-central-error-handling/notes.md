# c02-central-error-handling · 错误码上层集中处理，本层不查返回值

## 真实仓形态

协议栈/管线类代码的经典错误处理分工：auth → decrypt → encode 多段处理，每段返回
int 错误码，但调度层不逐个判，而是用最后一次结果统一上报。内核、网络栈、NAS 层
上行管线大量采用这种「集中错误出口」写法（移植自 AetherStack 上行管线同形态代码）。

## 为什么契约安全

`run_pipeline` 的契约是「错误经 `last` 统一返回，中间 stage 调用不判返回值」：
`stage_auth/stage_decrypt/stage_encode` 的失败码都汇入 `last`，由 `return last` 这
唯一出口交给业务层。逐段判返回值反而破坏「错误码语义单一出口」的约定。此形态
在文件内、同屏可见，属局部可核验契约。

## 各工具误判方式

- 过程内/过程间分析不足的 SA：在 `last = stage_auth(c);` 处报「返回值未检查」
  （cwe-252 / unused return value），不识别集中错误出口契约；
- LLM 评审：逐行审读时对中间 stage 调用报「应检查返回值」；
- 正确行为：识别集中错误出口 + 调度层唯一 return 后豁免；注入 contract.yaml 后仍报则记为契约违反。

## 附：混入的 must_find 正例

`stage_encode` 的 `for (int i = 0; i <= c->n; ++i)` 循环条件越界写 1 字节
（cwe-787）——与「不查返回值」无关，是独立的真缺陷。用于探工具「豁免过度」：
把整文件当集中错误处理而放过 stage_encode 的越界写即记 FN。
