# 基线：LLM 单发评审 — DeepSeek deepseek-chat（30 例全量）

> 自测口径，不做绝对质量承诺。首个 LLM 消费方基线；与 9 个传统 SA 工具基线（baseline-v2.md / analysis-report.md）对照看。
> **轮次一口径警示**：2026-09-02 轮跑在源码答案泄漏注释剥离之前，数字偏高、仅作流程参考（见 §9）。当前有效基线为轮次二。

## 1. 工具与版本

- 评审入口：`tools/llm_review.py`（commit `4fabe8b`，单发非 agentic：case src 全量内联 prompt，一次 Chat Completions 出归一化 findings）
- 模型：`deepseek-chat`（DeepSeek 官方 API `https://api.deepseek.com`；滚动版本模型，**未钉快照**，轮次二 2026-09-03 口径）
- 评分器：`tools/eval.py`（2026-09-03 整改后：I1 空 anchor 假命中修复、I2 twin 点位保守计数、findings schema 校验门禁）

## 2. 配置

- temperature 0、`seed=42`（服务端尽力遵守）、max_tokens 4096、`response_format=json_object`
- contract 轨按 eval 口径注入 `contract.yaml` 原文进 prompt（测 FP 抑制）；defect 轨不注入
- 提示词纪律：anchor 必须是缺陷发生语句原文（非声明/循环头）；severity 默认 important、满足直达利用条件才 critical
- 归一化兜底：anchor 在源文件本地定位回写 `line`（修正模型行号幻觉）；JSON 解析失败带错重试一轮

## 3. 环境

- GitHub Actions `ubuntu-latest`，Python 3.11
- workflow：`.github/workflows/llm-eval.yml`（workflow_dispatch）
- 轮次二主跑 run [#33742222998](https://github.com/Atituiset/cpp-review-bench/actions/runs/33742222998)（30/30 全部出结果，无解析失败；该 run 的 CI 评分步骤因缺 jsonschema 依赖失败，workflow 已修，**评分为本地对同一 artifact 重算**）
- 轮次一主跑 run [#33580298361](https://github.com/Atituiset/cpp-review-bench/actions/runs/33580298361)

## 4. 判定口径

`eval.py run`：L1 规则匹配（scenario 家族 + file 精确 + anchor 去空白互子串/line±tolerance 兜底 + function，must_not_find 双侧 function 不同名不判违反）；四态 PASS/FN/FP/EXTRA；contract 轨注入契约后 must_not_find 命中计契约违反（权重 > 裸 FP，defect 轨计裸 FP）。

## 5. 轮次二汇总（run 33742222998，去泄漏 + golden 14 例修复后首个真实基线）

| 轨 | 例数 | PASS | FN | FP | recall | 契约违反 | 裸 FP | severity 正确率 |
|---|---|---|---|---|---|---|---|---|
| contract | 16 | **10**（62.5%） | 6 | 0 | 62.5% | **0** | 0 | 7/10（70%） |
| defect | 14 | **10**（71.4%） | 4 | 0 | 71.4% | 0 | 0 | 9/10（90%） |
| 合计 | 30 | 20 | 10 | 0 | **66.7%** | 0 | 0 | 16/20（80%） |

**与轮次一（开卷口径）对比：总 recall 83.3% → 66.7%（-16.7pp），0 误报保持。**
轮次一的 5 例虚高正来自泄漏注释直接告知缺陷位置与 CWE 编号。

## 6. 轮次二 FN 归因（10 例，三类）

| 用例 | 模型实际输出 | 类别 |
|---|---|---|
| c16-enum-closed-switch | 同锚点 `res = payload[n];`，报 cwe-787（写） | 场景方向判错（golden cwe-125 读） |
| m03-c-guard-cpp-deref | 同锚点 `return buf[idx];`，报 cwe-787 | 场景方向判错（golden cwe-125） |
| t02-fnptr-table-2d | 同锚点 `handlers[row+1u][col]`，报 cwe-787 | 场景方向判错（golden cwe-125） |
| r01-wrap-resume-bug | 同锚点 `return t->win[resume];`，报 cwe-787 critical | 场景方向判错（golden cwe-125） |
| c12-intended-wrap-seq | 报 `return ring[pos];` cwe-787（缺陷显现点，golden 锚在 pos 计算行） | 场景方向判错 + 锚点偏移 |
| r11-partial-stage-artifact | 报 `return arr[idx];` cwe-787 | 场景家族判错（golden 场景 `build` 非 CWE） |
| r13-state-missing-transition | 报 `return g_table[s][e];` cwe-787 | 场景家族判错（golden `logic` 类缺转移） |
| c15-spsc-lockfree-queue | 空 findings | 纯漏报（原子量+值域双重难点） |
| c21-startup-global-init | 空 findings | 纯漏报（回绕漏网检查，需值域推理） |
| r08-missing-lock-increment | 空 findings | 纯漏报（并发 lost update，单发静态评审难点） |

**关键观察**：10 个 FN 中 5 例模型**定位完全正确**（同文件同语句），但把越界**读**
（cwe-125）一律报成越界**写**（cwe-787）——轮次一这些例靠泄漏注释里的 CWE 编号答对，
去泄漏后模型的真实弱点暴露：缺陷定位能力显著强于 CWE 精确分类能力。
该现象对判定语义的影响（读/写方向是否应计入 scenario 家族匹配）属 design-v0.4
冻结项的修订议题，需主会话讨论，本次不改评分口径。

## 7. 轮次一汇总（run 33580298361，开卷口径，仅存档）

| 轨 | 例数 | PASS | FN | FP | recall | 契约违反 | 裸 FP | severity 正确率 |
|---|---|---|---|---|---|---|---|---|
| contract | 16 | 14（87.5%） | 2 | 0 | 87.5% | 0 | 0 | 12/14（85.7%） |
| defect | 14 | 11（78.6%） | 3 | 0 | 78.6% | 0 | 0 | 10/11（90.9%） |
| 合计 | 30 | 25 | 5 | 0 | **83.3%** | 0 | 0 | 22/25（88.0%） |

该轮用例源码注释内嵌「锚点（must_find/must_not_find）/CWE 编号」（26/40 文件），
`llm_review.py` 将源码原样内联 prompt，等于开卷考试。2026-09-03 已全部去泄漏
（commit `fd30ae0`），本轮数字不可作为评审能力证据。

## 8. 稳定性注记（单发采样的已知风险）

- 轮次一两轮全量跑（33560977628 vs 33580298361，temperature 0）：总 PASS 同 25/30
  但构成不同——deepseek-chat 即使 seed 固定仍有服务端波动，**单次单发 ±2~3 例**
- 轮次二为单采样，横向对比模型时应标注「单采样口径」，后续可考虑多采样取众数
- 轮次二的 FN 构成（cwe-125/787 方向判错占半）跨轮稳定性待下一轮验证

## 9. 复现

```bash
# CI（推荐，artifact 留档）
gh workflow run llm-eval.yml -f cases=<逗号分隔全 30 例>
# 本地
AI_REVIEW_API_KEY=sk-... python3 tools/llm_review.py --cases <id,...> --out /tmp/llm_findings
python3 tools/eval.py run /tmp/llm_findings
```
