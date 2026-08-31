# CLAUDE.md — cpp-review-harvest 子目录（单仓版）

> 写给在本目录工作的 agent / 工程师。读本文件后再动手。本文件是「权威事实源」，与设计文档冲突时以较新的设计文档为准。
> 本目录是 `cpp-review-bench` 仓的**采集管线**子目录；仓根的 `cases/`（双轨用例）+ `schema/`（schema 冻结点）
> + `tools/eval.py`（四态评分）+ `sa/`（9 工具 runner/adapter）是本目录的输出目标与验证手段。

## 1. 这是什么

**候选线索生产线 + 人审移植流水线**：基于 GitHub CI（schedule 矩阵）自动采集开源 C/C++ 仓的真实缺陷
候选（draft = 线索 + 移植 blueprint，不是半成品用例），经「多工具共识 + 人审移植」转化为仓根 `cases/`
的标准用例——accept（confirm-tp/contract）= 承诺参照真实案例移植重写一个真实可编译用例
（自然风格、无播种标记）后入仓，不再是「确认即入仓」。与仓根评测管线构成**双管线闭环**
（采集 → 候选 → 进 cases/ 由 9 工具实测能否检出）。

核心设计：**一条管线，两头收货**——真缺陷候选 → `cases/defect/`；fp-mining 误报矿（可选开启）与
人审判 FP 的候选 → `cases/contract/`（真实世界契约案例，每条 FP 确认 = 一条 exemption_pattern 契约入库）。

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

1. ✅ `config/repos.yaml`（curl/sqlite/redis/nginx/vim/postgres/linux 7 仓 matrix，pinned ref、build 配置、路径白/黑名单）+ `config/rules.yaml`（规则 ID→scenario 映射、噪声黑名单、vote 规则）
2. ✅ `tools/pr_mine.py`（GitHub API 爬 PR diff+review → 切片 → judge 启发式）——**pr-mining 是当前唯一采集源**；sa-scan 路线的 `build_compdb.sh`、`scan_csa.sh`、`scan_cppcheck.sh` 为规划项未实现
3. ✅ `tools/normalize.py`、`vote.py`（单源退化 min-tools 1）、`pack_case.py`（五文件草稿）；`evidence.py` 为规划项未实现
4. ✅ 仓根 `.github/workflows/harvest.yml`（matrix: source × repo，`fail-fast: false`，artifacts，每日 cron）+ `harvest-package.yml`（inbox 三态流转辅助）+ `harvest-review.yml` + `harvest-pr-sarif.yml`（if:false 禁用中）
5. ⏳ 仓内联动验证：三态流转机制已实现（confirm-tp/contract 经 harvest-package.yml git mv 进 cases/，rejected 落 harvest/inbox/rejected/），confirmed/ 目录不使用
6. ✅ 定位修正轮（2026-08，线索 + blueprint）：
   - **license 策略**：候选 JSON 新增 `license`/`port`/`track_hint`/`polarity` 四顶层字段（pr_mine 产出、vote 透传、pack_case 消费；旧候选默认 unknown/rewrite/defect/must_find）；direct 组（curl/sqlite/nginx/postgres）可直接移植，rewrite 组（redis/vim/linux）只允许参考重写；license/port/源 PR 只进 notes.md 溯源表，不进 golden.json
   - **场景配额**：`pr_mining.max_per_scenario`（默认 5，CLI `--max-per-scenario`，兜底链 CLI > config > 5），同 scenario 桶满即跳过
   - **fp-mining**：`pr_mining.fp_mining`（默认 false）+ CLI `--fp-mining`，缺陷轮之后第二轮抓「修静态分析误报」PR，产出 track_hint=contract + polarity=must_not_find 候选
   - **blueprint notes**：pack_case.py 的 notes.md 改为移植 blueprint 六段（溯源表/缺陷描述与触发条件/真实修复 diff/移植要点/为什么契约安全/accept 检查清单六项）
   - **四态判定口径修正**：`make_draft_sarif.sh` 从 scenario 数字 grep（必不命中）改为 file+anchor 口径，与 eval.py L1 一致；harvest.yml propose 的 PR body 含 draft 定位说明、轻验证结论（eval_inbox_report.md）、候选溯源总览、accept 检查清单、审核指令
7. ✅ 少 stub 编译轮（2026-08，策略 1+2）：
   - **同文件闭包切片**：`pr_mining.closure`（默认 true，CLI `--no-closure`）——拉 base 完整文件，切片引用的同文件定义递归带上 + 按 libc 符号补标准头前导，零 stub 降编译成本
   - **可编译性打分**：候选新增 `dep_count`（启发式）与 `compile_errors`（gcc syntax-only 实测，权威编译地板）两顶层字段；pack_case 按 compile_errors → dep_count 升序打包，🟢 零依赖以 compile_errors==0 判定；curl 10 PR 实测 4 条 ≤3 错优先移植

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

- **已并入 cpp-review-bench 单仓**（原 cpp-review-harvest 独立仓设计已改写为仓内子目录版）；管线已实现并在 CI 跑过多轮
- **定位（2026-08 修正）**：候选线索生产线 + 人审移植流水线——draft = 线索 + 移植 blueprint（notes.md 六段 + 原始切片 src/），不是半成品用例；accept（confirm-tp/contract）= 承诺参照真实案例移植重写一个可编译用例后入 cases/，不再是「确认即入仓」
- 采集源现状：**pr-mining 是唯一实现的采集源**（爬 GitHub 历史 PR → 切片 → judge 启发式）；sa-scan（SA 扫描源码）为占位未实现
- 采集开关：场景配额 `pr_mining.max_per_scenario`（默认 5）；fp-mining `pr_mining.fp_mining`（默认 false，CLI `--fp-mining` 显式开启，产出 contract 轨 must_not_find 候选）；同文件闭包切片 `pr_mining.closure`（默认 true，CLI `--no-closure` 关）
- 候选 JSON 契约：`license`/`port`/`track_hint`/`polarity` 四顶层字段；license/port/源 PR 只进 notes.md 溯源表，不进 golden.json（golden schema context 为 additionalProperties:false，冻结不动）
- matrix 7 仓：curl/sqlite/redis/nginx/vim/postgres/linux；每日 cron（`23 3 * * *`）
- vote 单源退化：`--min-tools 1`（design 的 ≥2 工具共识待 sa-scan 上线后恢复）
- inbox 三态实际形态：draft/rejected 落盘 `harvest/inbox/`；confirm-tp/contract 动作经 harvest-package.yml git mv 进 `cases/`（语义上 = 移植重写完成后的入仓动作，不再是「确认即入仓」），`confirmed/` 目录不使用
- workflow 在仓根 `.github/workflows/`：harvest.yml（采集+vote+propose）、harvest-package.yml（三态流转）、harvest-review.yml、harvest-pr-sarif.yml（if:false 禁用中）、build-cooddy-image.yml
- **使用手册**：`harvest/README.md`「使用流程（端到端）」——采集（cron/dispatch 输入表）→ 候选 PR 结构 → 移植重写五步（先过 check_cases.py + 全量编译）→ `/case accept|contract|reject` 评论指令流转（机械 git mv、不校验移植完成度，故必须先移植后流转）→ merge 进 main 跑 9 工具四态
- v0.2 候选：sa-scan 采集源（CSA + CppCheck）、Infer 构建重待评估
