# SARIF PR 可视化 + 四态回流 实施方案

> 参照 `agent-reviewer@mvp` 的 `ai-review-reusable.yml` + `artifact-to-sarif.sh`（即 u-boot PR #3 的玩法）。
> 本仓最初初衷：用 cpp-review-bench 真实 bug 考题，让 9 个 SA 工具与 agent-reviewer（AI 评审）同台对比验证。

## 目标

1. **PR 态可视化友好**：harvest 候选 PR 上，每条候选的真实 bug 位置在代码行内联标注（GitHub code-scanning 原生 SARIF 渲染，等同 u-boot PR #3）。
2. **看得懂的 pass/fail**：PR 评论用大白话表格展示每条候选「被哪个工具标出 / 四态（TP/FN/FP/EXTRA）」，并明确「9 个 CI check 的 SUCCESS = 工具跑通，≠ 用例被检出」。
3. **未来对比验证（初衷）**：agent-reviewer@mvp 作为额外评审器跑在同一 cases/ 上，其 SARIF（category=agent-reviewer）与 9 工具 SARIF 并排上传 → 同台对比。

## 机制（源自 agent-reviewer 实证）

- `findings.json`（归一化 findings）→ `findings_to_sarif.py` → SARIF 2.1.0
- `github/codeql-action/upload-sarif@v3`（category 区分工具）→ PR 代码行内联标注 + code-scanning check
- PR 评论（人读层）：verdict + 每条 finding 的 scenario/severity/证据链

## 阶段 1（当前实施）

### 1.1 新增 `sa/adapters/findings_to_sarif.py`
- 输入：归一化 findings doc（`{tool, track, case_id, findings:[{file,line,scenario,severity,message,anchor,function,flow?}]}`）或裸 candidates 目录
- 输出：SARIF 2.1.0（ruleId=scenario；region.snippet 嵌源码行；message.markdown 嵌证据链/理由；partialFingerprints.findingIndex 去重）
- 复用 agent-reviewer 的 SARIF 形状（ruleId=场景键、rules 元数据、level 由 severity 映射）

### 1.2 harvest propose 阶段接入
- 收集 draft 后跑轻量 SA 评测环（`run_eval_inbox.sh`：clang --analyze + cppcheck 单文件）
- 每条候选：把轻量 SA 信号合成 findings.json → `findings_to_sarif.py` → 每候选一个 SARIF（category=harvest-csa / harvest-cppcheck）
- `upload-sarif` 上传 → PR 代码行内联标注（draft src 是新文件，会被标注）
- PR 评论：四态大白话表（候选 / scenario / CSA / CppCheck / 是否被标出 / 说明）

### 1.3 PR 评论四态语义（draft 阶段）
- 候选 golden.must_find[0].scenario = 我们猜的 bug 类型
- 轻量 SA 标出该 scenario → **TP（被标出）**；未标出 → **FN（漏报/静默）**
- 这是给人工审核的「客观依据」：你审 31 条候选时，每条旁有「clang 标出空指针 / cppcheck 静默」

## 阶段 2（accept 后，正式用例）
- ci.yml 9 工具评测产物 findings.json → `findings_to_sarif.py` → `upload-sarif`（category=各工具）
- code-scanning 显示每个正式用例被哪些工具标出
- eval.py 四态（PASS/FN/FP/EXTRA）回流 PR 评论 + 汇总报告

## 阶段 3（初衷：agent-reviewer 对比）
- cases/ 上 workflow_call 引用 `agent-reviewer/.github/workflows/ai-review-reusable.yml@mvp`
- 其产出 SARIF（category=agent-reviewer）与 9 工具并排 → AI 评审 vs 9 SA 同台
- 结合 eval.py golden 判四态 → 最终对比验证报告

## 注意
- draft 候选是外部仓片段，完整 9 工具编不过 → 阶段 1 用轻量 SA；完整 9 工具在 accept 进 cases/ 补编译后跑（阶段 2）
- upload-sarif 仅标注 PR 改动文件；draft src 是新文件，可标注
- SARIF 仅展示层，canonical findings.json 不变
