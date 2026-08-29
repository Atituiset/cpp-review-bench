# 证据：CppCheck（第二对照工具）

CppCheck 2.13 接入 CI（apt 直装），作为 CSA 之外的第二个独立 SA 对照。
JSON 由 `tools/cppcheck_to_findings.py` 从 CI artifact 落盘。

## 复现路径

- CI run：33259882689（bench-ci，main @ 6bd4f54）
- 工具：Cppcheck 2.13.0（Ubuntu apt），`--enable=warning,style,performance,portability,information --inconclusive --xml`
- 适配器已过滤 `information` 级噪声（include not found）并传入系统头 `-I`

## 结果（修复后，噪声已清）

| case | track | findings | must_find hit | bare_fp | 说明 |
|---|---|---|---|---|---|
| c01 | contract | 1 | 0/1 | 0 | style 噪声，未查到 cwe-190 |
| c02 | contract | 6 | 0/1 | 0 | 全 style（last 重赋值/const 参数），未查到 cwe-787；must_not_find(cwe-252) 未被误报 |
| c06 | contract | 0 | 0/1 | 0 | FAM 访问完全未碰，must_not_find 自然不报 |
| c08 | contract | 0 | 0/1 | 0 | 跨文件越界读未查到 |
| r10 | defect | 0 | 0/1 | 0 | 定长非字符串越界未查到 |

**recall=0，bare_fp=0**：CppCheck 默认 checker 集对语义型缺陷（整数回绕/不可信字段驱动偏移/定长非字符串越界）同样不敏感，与 CSA 一致。

## 互证含义

CSA + CppCheck 两工具对当前 5 例均为 recall=0 且无分歧（双工具都没查到 must_find、
都没误报 must_not_find）。这构成「传统 SA 基线对照」，但仍未形成跨工具分歧——
真正的分歧待 cooddy（CWE 原生 + 符号执行）与 Infer（抽象解释）接入后产生：
若 cooddy/Infer 能查到 c02/c06/c08 的 must_find 而 CSA/CppCheck 查不到，即证明
bench 的语义型缺陷确属传统 SA 盲区，且 contract 轨 FP 抑制可测性待对照验证。
