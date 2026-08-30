# CLAUDE.md — cpp-review-harvest 子目录（单仓版）

> 写给在本目录工作的 agent / 工程师。读本文件后再动手。本文件是「权威事实源」，与设计文档冲突时以较新的设计文档为准。
> 本目录是 `cpp-review-bench` 仓的**采集管线**子目录；仓根的 `cases/`（双轨用例）+ `schema/`（schema 冻结点）
> + `tools/eval.py`（四态评分）+ `sa/`（9 工具 runner/adapter）是本目录的输出目标与验证手段。

## 1. 这是什么

**benchmark 的自动化数据入口**：基于 GitHub CI（schedule 矩阵）自动采集开源 C/C++ 仓的真实缺陷候选，
经「多工具共识 + 人审管线」转化为仓根 `cases/` 的标准用例。与仓根评测管线构成**双管线闭环**
（采集 → 候选 → 进 cases/ 由 9 工具实测能否检出）。

核心设计：**一条管线，两头收货**——真缺陷候选 → `cases/defect/`；工具报了但人审判 FP 的 → `cases/contract/`
（真实世界契约案例，每条 FP 确认 = 一条 exemption_pattern 契约入库）。

## 2. 权威设计文档（按优先级读）

1. **`harvest/docs/design-v0.1.md`**（本目录）——管线六阶段、候选打包五文件草稿、人审三态、v0.1 范围与验收
2. **`harvest/docs/roadmap-pr-mining-pipeline.md`**——双管线（PR 爬取 + bench 评测闭环）综合规划、论文对标、单仓结构
3. `../docs/design-v0.4.md`（仓根 bench）——归一化 findings schema、golden schema（打包产物必须兼容）、inbox 治理
4. `~/Projects/vul-auto-private/gen-auto/readme.org` + `parse_sarif.py` / `sarif/materialize.py` / `vote.py`——多 SA 归一化与投票共识参照
5. `~/Projects/agent-reviewer/docs/design/high-fp-scenarios.md`——contract 侧产出场景分类依据

## 2.5 设计冻结点（协作军规，不可违反）

1. **契约冻结**：不得修改 `../docs/design-v0.4.md`（bench）的判定语义、golden schema、归一化 findings 格式，
   以及本目录 `design-v0.1.md` 的候选打包契约——采集产物靠 inbox/cases 对接，任何变更先回主会话讨论。
2. **用例先审**：前 3 个 case 产出后**必须先交审**（代码自然度、golden 锚点真实性、notes 三段式），通过后才批量产出。
3. **里程碑评审**：到达验收线才交付评审（bench v1 建成线 / harvest 端到端首跑 §6），中途不过程汇报。

## 3. 已有资产（直接复用）

| 资产 | 位置 | 用途 |
|---|---|---|
| 归一化 findings schema | 仓根 `schema/findings.schema.json`（与 `sa/adapters/*_to_findings.py` 同源） | normalize.py 输出契约 |
| SARIF 生成模式 | `agent-reviewer/scripts/artifact-to-sarif.sh` | 候选「证据链」字段来源 |
| codegraph（CLI/MCP） | `~/Projects/codegraph`（public） | evidence.py 链回溯素材 |
| navmap 提取器 | `~/Projects/navmap`（public） | 候选涉及分发表/全局量时结构化证据 |
| 9 工具评测环 | 仓根 `sa/runners/*` + `tools/eval.py` | 候选入 cases/ 后实测能否检出（飞轮闭环） |
| gen-auto 机制 | `~/Projects/vul-auto-private/gen-auto` | vote.py 共识判定、规则 ID→CWE 映射种子 |

## 4. v0.1 实施任务清单（验收线，按 design §6）

1. `config/repos.yaml`（aetherstack smoke + curl/sqlite/redis，pinned ref、build 配置、路径白/黑名单）+ `config/rules.yaml`（规则 ID→scenario 映射、噪声黑名单、vote 规则）
2. `tools/pr_mine.py`（GitHub API 爬 PR diff+review → 切片 → LLM judge 占位）+ `tools/build_compdb.sh`、`scan_csa.sh`、`scan_cppcheck.sh`
3. `tools/normalize.py`、`vote.py`、`evidence.py`、`pack_case.py`（五文件草稿）
4. `.github/workflows/harvest.yml`（matrix: source × repo，`fail-fast: false`，artifacts）+ `package.yml`（inbox 三态流转辅助）
5. 仓内联动验证：一次完整三态流转（confirm-tp / confirm-fp（contract.yaml 非空）/ reject 带原因）

## 5. 工程约定（继承母项目教训）

- **pinned ref 必须写入产物**：目标仓 commit/PR 钉死，任何候选可溯源到确切代码版本
- **失败隔离**：`fail-fast: false`；单仓/单 PR/单工具失败记 failure 跳过，不硬修、不拖垮整轮
- **规则 ID → scenario 映射**覆盖不了时进 `unmapped` 桶由人审归类，不硬猜；映射覆盖率进每轮报告
- **sed 无匹配不报错**：脚本改文件后用 grep 断言；u-boot 类仓 `.gitignore` 忽略点文件需 `git add -f`
- **无敏感信息**：文件不得含 GitHub 用户名/仓库地址（对外场景用 `<org>` 占位）、API key、邮箱、本机绝对路径；
  GitHub token 只走 `secrets.GITHUB_TOKEN`，不落盘
- 代码注释与文档用**中文**；提交信息中文 conventional 格式（`feat: / fix: / docs: / ci:`）；不写受众标签
- 构建失败的仓记 failure 跳过——不硬修目标仓构建系统
- **单仓约定**：本目录产物只进 `harvest/inbox/` → `cases/`，不跨仓；不修改仓根冻结点文件

## 6. 当前状态

- `harvest/docs/design-v0.1.md` 已定稿（单仓版，基于原独立仓设计改写）
- 双管线规划 `harvest/docs/roadmap-pr-mining-pipeline.md` 已落盘
- 实现尚未开始（本分支 `exp/harvest-pipeline` 起步）
- v0.1 先做 sa-scan（CSA + CppCheck）+ pr-mining（爬 PR）两采集源；Infer 构建重，v0.2 评估
