# 证据：Clang Static Analyzer（单 TU + 原生 CTU）

本目录归档 CI 上真实跑出的 findings 与评分汇总，作为 `reports/baseline-v1.md` 的可复现证据。
所有 JSON 由 `tools/csa_to_findings.py` + `tools/eval.py` 从 CI artifact 原样落盘，未手工修改。

## 复现路径（design §4.1：版本钉死）

- 执行环境：GitHub Actions `ubuntu-latest` + 官方 LLVM apt 源
  `clang-21` / `clang-tools-21`（含 `clang-extdef-mapping-21`）
- 工具版本：`Ubuntu clang version 21.1.8`
- 编译：`-std=c11 -Wall`（compdb 由 `cmake -S . -B build` 生成）
- 入口脚本：`consumers/local/run_csa.sh [singletu|ctu] <out>`
- CI workflow：`.github/workflows/ci.yml`（run 33258703246 —— CTU 真生效那次）
- 评测器：`tools/eval.py run <dir>`

## 关键证明：原生 CTU 确实生效（非退化）

CI 日志明确输出 `[ok] externalDefMap 生成: 7 行`（非 `[warn] ... 退化为单 TU`）。
即 `clang-extdef-mapping-21 gen` 成功产出跨 TU 外部定义映射，后续 `clang --analyze` 带
`-Xanalyzer -analyzer-config -Xanalyzer ctu-dir=... -Xanalyzer -analyzer-config -Xanalyzer experimental-enable-naive-ctu=true`
完成跨文件分析。结论不是"CTU 没跑"，而是"CTU 跑通了仍查不出"。

## 结果（两模式一致）

| 模式 | contract recall | defect recall | 裸 FP | 契约违反 |
|---|---|---|---|---|
| 单 TU 默认 | 0.0 | 0.0 | 0 | 0 |
| 原生 CTU | 0.0 | 0.0 | 0 | 0 |

三个 must_find 全部漏报（c01 cwe-190 回绕 / c08 cwe-125 跨文件越界读 / r10 cwe-125 奇数长度越界读），
FP 侧干净（未误报任何 must_not_find）。

## 文件清单

- `singletu/*.json`：各 case 单 TU 归一化 findings
- `ctu/*.json`：各 case 原生 CTU 归一化 findings
- `singletu_summary.json` / `ctu_summary.json`：对应 `tools/eval.py run` 汇总
