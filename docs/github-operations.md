# GitHub 上的运作方式（workflow 全景与界面地图）

> 本文回答两个问题：**本仓的自动化在 GitHub 上是怎么跑的**、**在 GitHub 界面上分别去哪个入口看结果**。
> 原理与评测协议见 `README.md`「原理与流程（入门导读）」与 `docs/design-v0.4.md`；SARIF 细节见 `docs/sarif-visualization-plan.md`。

## 运行位置

全部跑在 **GitHub-hosted runner（`ubuntu-latest`）** 上，本地零依赖、零构建。本地唯一入口是 `consumers/local/run.sh`，供开发机自助试分（可选）。

## 对内：本仓的 7 个 workflow（`.github/workflows/`）

| workflow | 触发 | 干什么 | 结果落在哪 |
|---|---|---|---|
| `ci.yml`（bench-ci） | push / PR 到 main + 手动 | 7 个并行 job 各跑一个工具（CSA singletu/CTU、CppCheck、clang-tidy、Infer、CodeQL、KLEE、CodeChecker、Joern、Cooddy 冒烟），归一化后 `eval.py` 四态评分 | Actions run 日志 + Artifacts（`all-findings` 等） |
| `llm-eval.yml` | 手动 dispatch（选 cases/model/base_url） | `tools/llm_review.py` 逐例评审 → eval.py 评分 → SARIF 上传 | run 页 **Step Summary 表格** + **Security → Code scanning**（category=`llm-eval`） |
| `harvest.yml` | 每日 cron（03:23 UTC）+ 手动 | scan（2 源 × 7 仓矩阵爬已合并 fix-PR）→ vote（归一化+共识+打包 draft）→ propose（轻量 SA 评测 + SARIF + **自动开候选 PR**） | **Pull requests**（标题「harvest: 每日候选 N 条待审核」）+ Code scanning（category=`harvest-draft`） |
| `harvest-review.yml` | issue_comment | 评论指令审核：候选 PR 下回 `/case accept\|contract\|reject <id>` → git mv 到 `cases/defect/` / `cases/contract/` / `inbox/rejected/` → push 回 PR 分支并回帖 | 候选 PR 的评论区 |
| `harvest-package.yml` | 手动 dispatch | inbox 三态流转的手动版（list / confirm-tp / contract / reject） | Actions 日志 |
| `harvest-pr-sarif.yml` | **已禁用**（`if: false`） | 原设计：SARIF 绑 PR head 做 Files changed 行内标注。实测 upload-sarif 在同仓 PR 上绑 default branch，**不会**在 PR 里画标注（GitHub 平台限制），故禁用，统一落 main 的 code scanning | — |
| `build-cooddy-image.yml` | 手动 dispatch | 构建 cooddy Docker 镜像推 GHCR，供 ci.yml 复用 | Packages |

### 门禁语义（两类红）

- **脚手架坏 = 红**：checkout / cmake 构建 / `check_cases.py` / `eval.py selftest` / findings 不合 schema / 工具缺失或崩溃。
- **评测零发现 = 不红**（fail-open）：分数是数据不是门禁。「9 个 CI check 全绿」只代表工具跑通了，不代表用例被检出——检出情况要看日志里的四态表。

## 对外：工具团队自助跑分（reusable workflow）

入口 `consumers/github-action/bench.yml`（`workflow_call`），消费方在自己仓写三行即可：

```yaml
jobs:
  bench:
    uses: <org>/cpp-review-bench/.github/workflows/bench.yml@v1
    with:
      tool_command: '<tool> analyze --compdb "$COMPDB" --out "$FINDINGS_DIR"'
```

输入：`tool_command`（必填，可用 `$COMPDB/$CASES_DIR/$FINDINGS_DIR/$BENCH_ROOT`）、`tool_image`（可选 Docker 镜像，命令在容器内跑）、`sarif_file`（可选，CodeQL 类工具免写 adapter）、`bench_ref`（钉 tag/commit 保可复现）。

流程：checkout bench 仓（pinned ref）→ cmake 全量构建 + compdb → 自检门禁 → 跑 `tool_command` →（可选 SARIF 归一化）→ `eval.py run` 评分 → 输出 artifact `bench-report-<tool>`（四态汇总 JSON + 全部 findings 明细）。

**现状**：GitHub 规定被 `uses:` 引用的 reusable workflow 必须物理位于本仓 `.github/workflows/` 下，该镜像**尚未发布**。现阶段接入需整份复制 `consumers/github-action/bench.yml` 到自己仓，用 `uses: ./.github/workflows/bench.yml` 本地引用。

## GitHub 界面地图（去哪看什么）

| 界面入口 | 能看到什么 | 来源 |
|---|---|---|
| Actions → run 日志 | 每工具 per-case 四态表 + 汇总 JSON；harvest 各 job 输出 | ci.yml / harvest.yml |
| Actions → run Summary | LLM 评审表格结果 | llm-eval.yml |
| Actions → Artifacts | `bench-report-*` / `all-findings` / `inbox-draft` 等可下载归档 | 各 workflow |
| Security → Code scanning | SARIF 告警列表，按 category 区分（`llm-eval` / `harvest-draft`） | upload-sarif |
| Pull requests | harvest 每日自动开的候选 PR；评论 `/case accept\|contract\|reject` 驱动审核流转 | harvest.yml + harvest-review.yml |

## 故障记录

- **2026-09-04 ~ 09-27 harvest 每日失败、无候选 PR 开出**：`81b7af4` 给 `sa/runners/run_eval_inbox.sh` 加了 clang/cppcheck 硬失败检查（工具缺失即 exit 127），但 `harvest.yml` 的 propose job 从未安装 cppcheck（`ubuntu-latest` 预装 clang、不含 cppcheck），评测环每日 exit 127，后续 SARIF 上传连挂。最后一个成功候选 PR 是 2026-09-03 的 #42。修复：propose job 补 `apt-get install cppcheck` 步骤（2026-09-27）。
