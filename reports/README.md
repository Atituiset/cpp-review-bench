# 基线报告登记（reports/）

本目录持续积累各工具的基线跑分。每条报告登记工具、版本、配置、环境，便于第三方复测对照。

## 当前权威报告（2026-09-03）

- **`baseline-v3-2026-09-03.md`**——9 工具 + LLM 总表与运行口径（CI run 33749944654 全绿可复现）。**各工具 recall/FP 数字一律以此为准**
- **`baseline-llm-deepseek-chat.md`**——LLM 轮次制基线（轮次二为去泄漏真实口径；轮次一开卷口径仅存档）
- **`audit-internal-2026-09-03.md`**——golden 内审记录（30 例复核、14 例修复），第三方标注审计的输入
- 历史存档（数字已被 v3 取代，仅留存沿革）：`baseline-v1.md`、`baseline-v2.md`、`analysis-report.md`

## 报告命名

`baseline-<tool>-<mode>.md`，如 `baseline-v1.md`（CSA 单 TU 默认）、`baseline-csa-ctu.md`（CSA 原生 CTU）。

## 报告必含字段（design §4.1 四条教训）

1. **工具与版本**：工具名 + 精确版本（clang 21.1.8 / agent-reviewer commit / 模型版本）钉死可复现。
2. **配置**：单 TU / CTU / L2 judge 模型与版本；checker 开关。
3. **环境**：编译器（gcc 13）、标准（`-std=c11 -Wall`）、OS。
4. **判定口径**：L1 匹配规则（scenario 家族 + file + anchor + line±tol + function）；四态定义；契约违反权重说明。
5. **汇总表**：per-track pass 率、recall、severity 正确率、裸 FP vs 契约违反分列、verified 计数。
6. **复现命令**。

## 公信力约定

- 所有数字标注「自测口径」，**不做绝对质量承诺**。
- 欢迎第三方复测 PR：fork → 跑 `sa/runners/run_csa.sh` 或接你的工具 → 提 `baseline-<tool>.md`。
- 标注主观性争议：golden 评审记录、合约豁免依据（contract.yaml）随仓公开。
