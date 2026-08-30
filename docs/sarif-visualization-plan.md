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

## 阶段 1 实施结果（2026-08-30 验证）
- ✅ `sa/adapters/findings_to_sarif.py`：归一化 findings → SARIF 2.1.0
- ✅ `harvest/tools/make_draft_sarif.sh`：轻量 SA（clang --analyze + cppcheck）逐候选生成 findings + 溯源表
- ✅ `harvest.yml` propose 步骤：生成 + `upload-sarif@v4` 上传（main 检出=修复脚本，确定性）
- ✅ **深历史扫描真正跑通**（run 33325819768）：curl/redis/vim/nginx 4 仓命中 redis 300 / nginx 144 / vim 139 / curl 97 PR，产出 **343 候选**，SARIF `（343 results, 3 rules）` 上传成功
- ✅ Security → Code scanning 可视化（343 results 上传，去重后唯一告警在 Security tab）
- ⚠️ **同仓 PR 内联标注限制**：upload-sarif 对同仓 PR 绑 default branch（refs/heads/main），不在 PR Files changed 画标注（平台限制，非 bug）
- ⚠️ **SARIF 参数 bug 已修**：原 `make_draft_sarif.sh` 用 `sys.argv` 位置传参，个数不匹配导致每候选 `ValueError`、0 results；改为**环境变量传参**后正常

## 阶段 1 踩坑（已修，留档）
1. **`in:title` 太窄**：原 query `fix in:title` 命中极少（curl 3、sqlite/postgres/linux 0）；改**全字段搜索** `fix OR bug OR leak OR overflow OR crash` 后量级对了（curl 97、redis 3056）。噪声由 judge_bug 启发式 + 人审兜底。
2. **config 覆盖 max_prs（关键）**：`pr_mine.py` 原 `args.max_prs = pm.get("max_prs_per_run", args.max_prs)` 用 config 的 50 **永远覆盖** workflow 传的 `--max-prs`，导致深历史一直只爬 50 PR/仓。改为 `args.max_prs = args.max_prs or pm.get("max_prs_per_run")`（input 优先）→ max_prs=300 真正生效，redis 爬满 300。
3. **sqlite/postgres/linux 搜索返回 0**：单关键词 fix/bug/leak 实测也是 0，非 query 问题。根因=仓特性：sqlite 大量改动直接 commit 不走 PR、linux 走邮件列表+直接 commit（GitHub PR 极少）、postgres 索引覆盖差。这三仓**天然 PR 少**，非 blocking；curl/redis/vim/nginx 才是真正用 GitHub PR 流程的仓，已扫到深历史。

## 阶段 1 结论
- 可视化友好目标**已达成**（Security tab 原生 SARIF 渲染；343 候选数据在 `harvest/inbox/draft/` + PR 溯源表）
- 路径可追溯：PR 溯源表每行带源 PR 链接，notes.md 内嵌真实修复 diff + accept 流程
- 候选 scenario 标「待定（非真值）」，不猜真值误导；正式仓手动 LLM 评审定真值

## 待确认
- 阶段 2：accept 进 cases/ 后完整 9 工具评测 + eval.py 四态回流
- 阶段 3：agent-reviewer@mvp 对比（初衷，手动触发，不烧 token）

## 注意
- draft 候选是外部仓片段，完整 9 工具编不过 → 阶段 1 用轻量 SA；完整 9 工具在 accept 进 cases/ 补编译后跑（阶段 2）
- SARIF 仅展示层，canonical findings.json 不变
- 当前候选 PR：#32（深历史 343 条，主力）、#27（76 条，对照）、#24（32 条两仓，对照）
