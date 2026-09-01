# 基线：LLM 单发评审 — DeepSeek deepseek-chat（30 例全量）

> 自测口径，不做绝对质量承诺。首个 LLM 消费方基线；与 9 个传统 SA 工具基线（baseline-v2.md / analysis-report.md）对照看。

## 1. 工具与版本

- 评审入口：`tools/llm_review.py`（commit `ef35374`，单发非 agentic：case src 全量内联 prompt，一次 Chat Completions 出归一化 findings）
- 模型：`deepseek-chat`（DeepSeek 官方 API `https://api.deepseek.com`；滚动版本模型，**未钉快照**，2026-09-01 复测口径）
- 评分器：`tools/eval.py`（commit `5a65323`，含 must_not_find function 冲突排除修复）

## 2. 配置

- temperature 0、max_tokens 4096、`response_format=json_object`
- contract 轨按 eval 口径注入 `contract.yaml` 原文进 prompt（测 FP 抑制）；defect 轨不注入
- 提示词纪律：anchor 必须是缺陷发生语句原文（非声明/循环头）；置信纪律「只报有证据缺陷」

## 3. 环境

- GitHub Actions `ubuntu-latest`，Python 3.11（stdlib urllib，无第三方依赖）
- workflow：`.github/workflows/llm-eval.yml`（workflow_dispatch），run [#33560977628](https://github.com/Atituiset/cpp-review-bench/actions/runs/33560977628)（29 例）+ [#33562257935](https://github.com/Atituiset/cpp-review-bench/actions/runs/33562257935)（c21 补跑；首次该例模型输出未含 JSON 对象，解析失败，属 LLM 输出偶发）

## 4. 判定口径

`eval.py run`：L1 规则匹配（scenario 家族 + file 精确 + anchor 去空白互子串/line±tolerance 兜底 + function，must_not_find 双侧 function 不同名不判违反）；四态 PASS/FN/FP/EXTRA；contract 轨注入契约后 must_not_find 命中计契约违反（权重 > 裸 FP，defect 轨计裸 FP）。

## 5. 汇总

| 轨 | 例数 | PASS | FN | FP | recall | 契约违反 | 裸 FP | severity 正确率 |
|---|---|---|---|---|---|---|---|---|
| contract | 16 | **12**（75%） | 4 | 0 | 75.0% | **0** | 0 | 3/13（23.1%） |
| defect | 14 | **13**（92.9%） | 1 | 0 | 92.9% | 0 | 0 | 2/13（15.4%） |
| 合计 | 30 | 25 | 5 | 0 | **83.3%** | 0 | 0 | 5/26（19.2%） |

对照：传统 SA 9 工具全部 30 例 must_find recall = 0（baseline-v2.md）。LLM 单发评审在本 bench 上把 recall 拉到 83%，且 **0 误报**（contract 轨 16 例「看似有缺陷、契约上安全」的陷阱全部正确抑制）。

## 6. FN 与扣分点归因（5 例未 PASS）

| 用例 | 现象 | 归因 |
|---|---|---|
| c02-central-error-handling | 报对缺陷、挂错语句（挂循环体 `c->buf[i]=i;`，golden 锚循环头 `for(...i<=c->n)`）；且模型给的 line=18 vs 实际 30，**行号幻觉**致 ±3 兜底失效 | 模型锚点/行号精度 |
| c06-flexible-array-member | 同型：挂循环体 `dst->data[i]=src[i];`，line=24 vs 实际 31 | 模型锚点/行号精度 |
| t01-fnptr-table-1d | 同一 anchor `handlers[idx+1u]=h_a;` golden 要 **cwe-125（越界读）**，模型报 cwe-787（写）→ 家族不匹配记 EXTRA | 场景家族判错 |
| c21-startup-global-init | golden 要 cwe-190 回绕道（`uint8_t end = off+1u;`），模型报下游后果 `g_buf[end]=0xFF;` cwe-787 | 报后果未报根因 |
| r11-partial-stage-artifact | golden 场景为 `build`（阶段工件残留，非 CWE），模型从内存安全角度报 cwe-787 | 场景家族判错 |

另：severity 正确率仅 19% —— 模型倾向报 critical，golden 多为 important（分级口径差异，非检出能力问题）。

## 7. 复现

```bash
# CI（推荐，artifact 留档）
gh workflow run llm-eval.yml -f cases=<逗号分隔全 30 例>
# 本地
AI_REVIEW_API_KEY=sk-... python3 tools/llm_review.py --cases <id,...> --out /tmp/llm_findings
python3 tools/eval.py run /tmp/llm_findings
```
