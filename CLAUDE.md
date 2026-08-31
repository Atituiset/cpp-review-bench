# CLAUDE.md — cpp-review-bench 项目交接与工程约定

> 写给在本仓工作的 agent / 工程师。读本文件后再动手。本文件是「权威事实源」，与设计文档冲突时以较新的设计文档为准。

## 1. 这是什么

**C/C++（含混合编程）代码评审基准**，双轨：Contract Track（测 FP 抑制）+ Defect Track（测 TP 检出）。军规：**一切用例都是真实可编译的 C/C++ 代码**（不是题目描述）。与任何检视工具（SA/LLM）和上下文工具（navmap/codegraph/clangd/CSA）解耦，消费形态四种（本地直跑 / CI 消费者 workflow / Agent PR 评审 / compdb 提取）。

相关仓（都在 `~/Projects/` 下）：

| 仓 | 关系 |
|---|---|
| `agent-reviewer` | 研发母仓：场景库 SKILL、CI 可复用 workflow、SARIF 转换器、试验归档 trials/、调研与设计文档 |
| `cpp-review-bench`（本仓） | 场景资产 + golden + 评分器 + sa/ 归一层 + harvest/ 采集管线（原独立仓 `cpp-review-harvest` 已并入本仓 `harvest/` 子目录，单仓开发） |

## 2. 权威设计文档（按优先级读）

1. **`docs/design-v0.4.md`**（本仓）——军规、双轨、golden schema v2（must_find = scenario+severity+file+anchor+function+rationale）、两层匹配评测协议、消费形态矩阵、30 例清单、建成线
2. **`harvest/docs/design-v0.1.md`**（本仓）——自动入库管线与三态人审
3. `~/Projects/agent-viewer-research/docs/design/high-fp-scenarios.md`——四篇 70 条误报地图（场景素材目录）
4. `~/Projects/open-code-review/papers/cpp-review-benchmark-research.md`——三层金字塔、golden comment 规范、标注协议（Defect Track 二期真实 CVE 轨）
5. `~/Projects/open-code-review/papers/cpp-review-benchmark-plan-assessment.md`——成本/风险/验收
6. `~/Projects/open-code-review/docs/src/appendix/benchmark.md`——第三方复测四教训（版本钉死/子集分开/口径公开/recall 粒度）

## 2.5 设计冻结点（协作军规，不可违反）

1. **契约冻结**：不得修改 `design-v0.4.md`（bench）的判定语义、golden schema、归一化 findings 格式，以及 `harvest/docs/design-v0.1.md`（harvest）的候选打包契约——单仓内靠 `harvest/inbox/` → `cases/` 目录与 schema 对接，任何变更必须先回主会话讨论，单方私改即断链
2. **用例先审**：前 3 个 case 产出后**必须先交审**（军规执行：代码自然度、golden 锚点真实性、notes 三段式），通过后才允许批量产出——风格跑偏只在规模前拦得住
3. **里程碑评审**：到达验收线才交付评审（bench v1 建成线 §7.1 / harvest 端到端首跑 §6），中途不需要过程汇报；未达验收线的半成品不进入评审

## 3. 已有资产（直接复用，不要重造）

| 资产 | 位置 | 用途 |
|---|---|---|
| 场景种子代码 | AetherStack 仓 `trial/s{1-4}-m{1,2,3}` 分支（12 个） | S1 单函数越界 / S2 同文件契约 / S3 跨文件直调 / S4 跨文件表断链，自然风格已编译通过——**已决定：不再单独触发试验 PR，直接移植为本仓 case src 的起点**（验证统一由本仓基线报告承担） |
| navmap 提取器 | `~/Projects/navmap`（public） | 分发表（结构体/裸一维/裸多维/using）/状态机/全局变量读写提取；`navmap_expect` 的产物形态以它为准 |
| SARIF 转换器 | `agent-reviewer/scripts/artifact-to-sarif.sh` | findings→SARIF 的成熟映射（codeFlows/region.snippet/message.markdown 嵌函数体）；归一化 findings schema 与其工件 schema 同源 |
| CI 可复用 workflow 模式 | `agent-reviewer/.github/workflows/ai-review-reusable.yml` | consumers/github-action 的参照（豁免/防重置/fail-open/pinned ref） |
| 门禁自测模式 | `agent-reviewer/scripts/selftest.sh`（22 例沙箱自测） | eval.py 自测的写法参照 |
| 试验归档结构 | `agent-reviewer/trials/` | reports/ 归档格式参照 |
| SA 归一层 | 本仓 `sa/`（adapters/ 9 个 `*_to_findings.py` + `findings_to_sarif.py`；runners/ 7 个 `run_*.sh`；scripts/joern/scan.sc；harnesses/ 11 个 KLEE harness；docker/cooddy/） | 9 工具扫描 → 归一化 findings → eval.py 评分的工程化入口，CI（ci.yml）与本地复现共用 |

## 4. v1 实施任务清单（建成线，按 design-v0.4 §7.1）

1. ✅ `cases/contract/` 16 例 + `cases/defect/` 14 例：五文件齐备（src/CMakeLists/golden.json/contract.yaml(仅 contract 轨)/notes.md 三段式），自然风格代码，无「试验/播种」字样
2. ✅ `schema/golden.schema.json` + `schema/findings.schema.json`，全部 golden 过校验
3. ✅ `cmake/AllCases.cmake`：一键全量构建 + 统一 `compile_commands.json`
4. ✅ `tools/eval.py`：两层匹配（L1 规则：scenario 家族/file/anchor 去空白/line±tolerance；L2 可选 judge），输出 per-case 四态（PASS/FP/FN/EXTRA）+ 汇总（per-track pass 率、裸 FP vs 契约违反分列、severity 正确率、verified 计数）；对构造 findings（故意 1FP+1FN+1 契约违反）判定正确
5. ❌ `consumers/` 未建（github-action/bench.yml 与 local/ 入口均未实现；现由 `.github/workflows/ci.yml` 7 个 job + `sa/runners/*` 承担工具接入）
6. ✅ `README.md`（军规置顶 + 双轨 + 快速上手）、`LICENSE`(Apache-2.0)、`CONTRIBUTING.md`、`CITATION.cff`、`reports/README.md`
7. ✅ 基线：`reports/baseline-v1.md`（CSA，3 例早期子集）、`baseline-v2.md`（4+ 工具全 30 例）、`analysis-report.md`（9 工具）

## 5. 工程约定（从母项目继承的教训，别再踩）

- **sed 无匹配不报错**：脚本里修改文件后用 grep 断言目标存在，别用 `||` 兜底 sed
- **u-boot 仓 `.gitignore` 忽略点文件**：`.ai-review-mode` 等需 `git add -f`
- **SARIF 细节**：codeFlows 的 message 放 `location` 对象内；`region.startLine ≥ 1`（模型可能给 0，转换时钳制）
- **workflow 权限**：可复用 workflow 需要 `id-token: write`（OIDC）+ `pull-requests: write`（评论）；caller 与 reusable 都要声明
- **claude-code-action 会重置 workspace**：跨步骤资产落 `/tmp`；场景库等工具仓步骤后重新 checkout
- **无敏感信息**：任何文件不得含 GitHub 用户名/仓库地址（对外分享场景下用 `<org>` 占位）、API key、邮箱、本机绝对路径
- 代码注释与文档用**中文**；提交信息用中文 conventional 格式（`feat: / fix: / docs: / ci:`）；不写「领导/面试官」等受众标签
- 每个用例必须编译通过才准提交；golden 的 anchor 必须在 src 中真实存在（eval.py 自检项）

## 6. 当前状态

- **cases/ 30 例已铺满**（contract 16 + defect 14），五文件齐备、全量编译通过、golden 全过 schema 校验
- **9 工具基线已跑**（CSA singletu/CTU、CppCheck、clang-tidy、Infer、CodeQL、CodeChecker、KLEE、Joern、Cooddy），报告见 `reports/baseline-v1.md` / `baseline-v2.md` / `analysis-report.md`；注意 KLEE/Joern 的 adapter 存在 golden 自证问题（修复中），其 recall 数字待重跑
- **harvest 采集管线已并入本仓**（`harvest/` 子目录，单仓开发）：pr-mining 采集源已实现并跑过多轮（matrix 7 仓：curl/sqlite/redis/nginx/vim/postgres/linux，每日 cron）；sa-scan 源为占位未实现；workflow 在仓根 `.github/workflows/`（harvest.yml / harvest-package.yml / harvest-review.yml / harvest-pr-sarif.yml（if:false 禁用中）/ build-cooddy-image.yml）。2026-08 定位修正：harvest 是「候选线索生产线 + 人审移植流水线」——draft = 线索 + 移植 blueprint（非半成品用例），accept = 承诺移植重写一个可编译用例后入 cases/；新增 license/port 策略与场景配额，fp-mining（contract 轨误报矿）可选开启
- 待办：`consumers/` 消费入口未建、`cases/calibration/` 为规划项、标注审计未做、KLEE/Joern 基线待 adapter 修复后重跑
- 12 个场景种子分支在 AetherStack（与本仓 S 级用例有重叠，已移植对齐）
- 格式上输出 Martian 兼容报表（便于与既有 PR 级评测交叉对照）仍为 v1.1 规划项
