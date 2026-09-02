# 基线：LLM 单发评审 — DeepSeek deepseek-chat（30 例全量）

> 自测口径，不做绝对质量承诺。首个 LLM 消费方基线；与 9 个传统 SA 工具基线（baseline-v2.md / analysis-report.md）对照看。

## 1. 工具与版本

- 评审入口：`tools/llm_review.py`（commit `4fabe8b`，单发非 agentic：case src 全量内联 prompt，一次 Chat Completions 出归一化 findings）
- 模型：`deepseek-chat`（DeepSeek 官方 API `https://api.deepseek.com`；滚动版本模型，**未钉快照**，2026-09-02 复测口径）
- 评分器：`tools/eval.py`（commit `5a65323`，含 must_not_find function 冲突排除修复）

## 2. 配置

- temperature 0、`seed=42`（服务端尽力遵守）、max_tokens 4096、`response_format=json_object`
- contract 轨按 eval 口径注入 `contract.yaml` 原文进 prompt（测 FP 抑制）；defect 轨不注入
- 提示词纪律：anchor 必须是缺陷发生语句原文（非声明/循环头）；severity 默认 important、满足直达利用条件才 critical
- 归一化兜底：anchor 在源文件本地定位回写 `line`（修正模型行号幻觉）；JSON 解析失败带错重试一轮

## 3. 环境

- GitHub Actions `ubuntu-latest`，Python 3.11（stdlib urllib，无第三方依赖）
- workflow：`.github/workflows/llm-eval.yml`（workflow_dispatch），主跑 run [#33580298361](https://github.com/Atituiset/cpp-review-bench/actions/runs/33580298361)（30/30 全部出结果，无解析失败）
- 前一轮预跑（锚点纪律修复前）run [#33560977628](https://github.com/Atituiset/cpp-review-bench/actions/runs/33560977628) + c21 补跑 [#33562257935](https://github.com/Atituiset/cpp-review-bench/actions/runs/33562257935)，见 §7 稳定性注记

## 4. 判定口径

`eval.py run`：L1 规则匹配（scenario 家族 + file 精确 + anchor 去空白互子串/line±tolerance 兜底 + function，must_not_find 双侧 function 不同名不判违反）；四态 PASS/FN/FP/EXTRA；contract 轨注入契约后 must_not_find 命中计契约违反（权重 > 裸 FP，defect 轨计裸 FP）。

## 5. 汇总（run 33580298361）

| 轨 | 例数 | PASS | FN | FP | recall | 契约违反 | 裸 FP | severity 正确率 |
|---|---|---|---|---|---|---|---|---|
| contract | 16 | **14**（87.5%） | 2 | 0 | 87.5% | **0** | 0 | 12/14（85.7%） |
| defect | 14 | **11**（78.6%） | 3 | 0 | 78.6% | 0 | 0 | 10/11（90.9%） |
| 合计 | 30 | 25 | 5 | 0 | **83.3%** | 0 | 0 | 22/25（88.0%） |

对照：传统 SA 9 工具全部 30 例 must_find recall = 0（baseline-v2.md）。LLM 单发评审在本 bench 上把 recall 拉到 83%，且 **0 误报**（contract 轨 16 例「看似有缺陷、契约上安全」的陷阱全部正确抑制）。

## 6. FN 归因（5 例，三类）

| 用例 | 现象 | 类别 |
|---|---|---|
| c21-startup-global-init | golden 要 cwe-190 回绕道（`uint8_t end = off+1u;`），模型报下游后果 `g_buf[end]=0xFF;` cwe-787 | 报后果未报根因 |
| r07-alloc-size-wrap | golden 锚分配点 `size_t total = (size_t)(n*size);`（组合 cwe-190+cwe-787），模型报后果 `p[i] = 0;` | 报后果未报根因 |
| t01-fnptr-table-1d | 同一 anchor `handlers[idx+1u]=h_a;`，golden 要 **cwe-125（越界读）**，模型报 cwe-787（写），家族不匹配 | 场景家族判错 |
| r11-partial-stage-artifact | golden 场景为 `build`（阶段工件残留，非 CWE），模型从内存安全角度报 cwe-787 | 场景家族判错 |
| r13-state-missing-transition | 空 findings——`logic` 类状态机缺转移缺陷，单发评审整体漏掉 | 纯漏报 |

## 7. 稳定性注记（单发采样的已知风险）

两轮全量跑（33560977628 vs 33580298361，均为 temperature 0）：

- **总 PASS 数同为 25/30、总 recall 同为 83.3%，但构成不同**：锚点/行号修复使 c02、c06 翻正；r07、r13 由 PASS 翻 FN（模型非确定性，deepseek-chat 即使 seed 固定仍有服务端波动）
- severity 正确率 23%→88%：分级纪律提示有效
- c21 首次跑模型输出非 JSON（解析失败），修复重试后正常——LLM 输出格式不稳定是常态，管线必须有兜底
- 结论：**单次单发数字有 ±2~3 例波动**，横向对比模型时应标注「单采样口径」，后续可考虑多采样取众数

## 8. 复现

```bash
# CI（推荐，artifact 留档）
gh workflow run llm-eval.yml -f cases=<逗号分隔全 30 例>
# 本地
AI_REVIEW_API_KEY=sk-... python3 tools/llm_review.py --cases <id,...> --out /tmp/llm_findings
python3 tools/eval.py run /tmp/llm_findings
```
