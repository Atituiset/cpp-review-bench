# 证据：阶段 A 试点 · contract 轨新增 c02 + c06（CI 互证）

本目录归档阶段 A 试点（contract 轨新增 2 例）在 CI 上的真实 findings 与评分汇总。
JSON 由 `tools/csa_to_findings.py` + `tools/eval.py` 从 CI artifact 原样落盘，未手工修改。

## 复现路径

- CI run：33259051637（`.github/workflows/ci.yml`，main @ 0222d6b）
- 工具：Ubuntu clang 21.1.8，单 TU + 原生 CTU 两种模式
- 入口：`consumers/local/run_csa.sh [singletu|ctu] <out>` → `tools/eval.py run <out>`

## 本阶段 CI 互证项（全部通过）

1. 全量编译 + 统一 compdb：新两例 src 进入 compdb（共 6 源）
2. `tools/check_cases.py`：5/5 case 过 schema + anchor/file 真实存在
3. `tools/eval.py selftest`：构造 findings 1FP+1FN+1契约违反 判定正确
4. CSA 单 TU + 原生 CTU 评测：新两例被评分器正确纳入

## CSA 实测（单 TU / CTU 一致）

| case | track | state | must_find hit | bare_fp | contract_viol |
|---|---|---|---|---|---|
| c01 | contract | FN | 0/1 | 0 | 0 |
| c02 | contract | FN | 0/1 | 0 | 0 |
| c06 | contract | FN | 0/1 | 0 | 0 |
| c08 | contract | FN | 0/1 | 0 | 0 |
| r10 | defect | FN | 0/1 | 0 | 0 |

CSA 对 c02/c06 的 must_find（cwe-787 越界写）同样漏报（recall 0），对 must_not_find
（cwe-252 未查返回值 / cwe-125 FAM 访问）未误报（bare_fp 0）。

## 诚实声明：本阶段为「单工具互证」，尚未形成跨工具对照

阶段 A 只有 CSA 一个工具。CSA 默认不敏感，对 contract 轨 must_not_find 自然不报，
因此 bare_fp=0 仅说明「CSA 不误报」，不能证明 c02/c06 的 must_not_find 真能钓出
过度敏感的 SA。**真正的互证（工具间分歧）待阶段 C 接入第二个工具（如 CppCheck/
Infer/某 LLM 评审）后形成**——届时对比两工具在 c02/c06 must_not_find 点的命中差异，
才能验证 contract 轨的 FP 抑制可测性。本目录保留 CSA 基线，作为后续对照起点。
