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
- 待办：`consumers/` 通用消费入口未建（llm-eval.yml 已先承担 LLM/Agent 消费：tools/llm_review.py 单发评审→归一化 findings→eval.py，workflow_dispatch 自选 cases/model/base_url）、`cases/calibration/` 为规划项、标注审计未做、KLEE/Joern 基线待 adapter 修复后重跑
- 12 个场景种子分支在 AetherStack（与本仓 S 级用例有重叠，已移植对齐）
- **LLM 基线首跑**（2026-09-02，DeepSeek deepseek-chat 单发，30 例全量）：recall 83.3%（contract 87.5% / defect 78.6%）、0 误报、severity 正确率 88%，报告 `reports/baseline-llm-deepseek-chat.md`（含两轮稳定性注记：单采样 ±2~3 例波动）；同轮修复 `eval.py` must_not_find function 冲突排除（commit 5a65323）与 llm_review 行号幻觉修正/分级纪律/解析重试（commit 4fabe8b）。**注意：该轮基线跑在源码内嵌标注注释剥离之前（见下），数字受答案泄漏影响，仅作流程参考，去泄漏后需重跑**
- 格式上输出 Martian 兼容报表（便于与既有 PR 级评测交叉对照）仍为 v1.1 规划项
- **质量整改一轮**（2026-09-03，外部评审发现的问题修复）：
  - B1 答案泄漏：cases/ 下 29 个源文件内嵌的「锚点（must_find/must_not_find）/真实缺陷/安全点/CWE 编号」注释已全部剥离改写为中性领域注释（代码行零改动，check_cases 仍全过）；此前 LLM/Agent 轨等于开卷考试
  - B2 评论注入：harvest-review.yml（issue_comment 触发、持 contents:write+PAT）及 harvest.yml/harvest-package.yml/llm-eval.yml 的用户可控输入全部改走 env 传值 + 白名单校验，消除 run 脚本插值注入面
  - B3 r01 伪缺陷重写：原 r01 声称的「(tx_next-ack) 回绕致恢复点错乱（cwe-190）」在代数上恒不成立（模 256 环上 ack+(tx_next-ack)≡tx_next，且 uint8_t 索引 256 项数组不可能越界），已重写为真缺陷——模 256 序号直接索引 64 项环形窗口未取模，越界读（cwe-125），安全点改为 rlc_fill 的取模索引；涉及 r01 的既有工具基线数字失效，待重跑
  - I1/I2/S1 + findings schema 门禁：修复 I1（finding 无 anchor 时 `""` 恒为子串导致 must_find 假命中，改为只能走 line±tolerance）、I2（twin 点位——must_find/must_not_find 同 file+anchor 仅靠 function 区分，如 r04/r09/r14——finding 无 function 时只计 FP 不计 TP）、S1（`run` 目录扫描遇缺 track/case_id 的 json 由 KeyError 改为跳过告警）；`eval.py run` 入口默认做 schema/findings.schema.json 校验（`--no-validate` 兜底）；新增 `tools/check_evidence.py`（reports/evidence 归档校验，已挂 CI build-and-eval 门禁，summary 产物豁免）与 `tools/normalize_evidence.py`（一次性归档归一化，已执行：114 个 findings 文件 100% 合规；注意 joern/klee/codeql/codechecker/infer adapter 仍可能产出不带 anchor 的 findings，重跑基线前需修 adapter）
  - 整改后新增待办：LLM 基线去泄漏重跑、r01 相关工具基线重跑、上述 5 个 adapter 补 anchor 输出
- **质量整改二轮**（2026-09-03 下午）：
  - adapter：5 个 adapter（joern/klee/infer/codechecker/sarif）补 anchor 三级合成，公共逻辑抽 `sa/adapters/_common.py`（SEVERITY_MAP 5 份复制消除）；修 KLEE `out of bound` 文案匹配丢失 scenario 的既有 bug、cppcheck 漏收 .cpp
  - CI：fail-open 收敛（工具失败=红、零发现不红、CTU 退化落 DEGRADED+::warning::）、版本钉死（klee v3.2、joern 按 digest、codechecker==6.28.3）、run_infer.sh 互斥模式混用修复、runner 卫生（执行位/set -euo pipefail/mktemp）
  - consumers/ 通用消费入口已建成：`consumers/local/run.sh` + `consumers/github-action/bench.yml` + 中文 README（README 消费形态表已指向）
  - **golden 内审 30 例 + 14 例修复**（记录 `reports/audit-internal-2026-09-03.md`）：伪缺陷 3 例重写（c12 环改 64 项标 cwe-125、r07 清零循环改 size_t 域、r09 删错误路径置 NULL）；场景标错 6 例纠正（c01b/c03/t01/r02/r05 改 cwe-787；r12 改 cwe-125 并按 gcc 实测纠正审计方案——16u 会造伪缺陷，源码未动，机理重写为缺下界检查，目录名 r12-signed-unsigned-compare 与新机理不符但未改）；存疑 5 例修复（c15 改 _Atomic+acquire/release、dequeue 取模；c21 GBUF_N 改 128+豁免锚点避开缺陷行；r08 补 join happens-before 契约；r11 活动树加 r11_get_ok 作 FP 探针；r13 EV_A/B 填真实 handler 仅 EV_C 缺失）。全部经 ASan/pthread 构造性验证，30 例 check_cases/selftest/全量构建通过
  - 待办更新：上述 14 例 scenario/机理口径变化涉及的既有工具基线数字全部待重跑（与 18:00 基线重跑合并进行）
- **基线重跑一轮**（2026-09-03 晚，去泄漏+14 例修复后首个有效基线）：
  - LLM（deepseek-chat 单发，run 33742222998，本地对 artifact 重算评分）：**recall 66.7%（contract 62.5%/defect 71.4%）、0 误报、severity 80%**——旧轮 83.3% 系开卷口径仅存档。FN 归因关键发现：10 个 FN 中 5 例定位完全正确但把 cwe-125（读）报成 cwe-787（写）——去泄漏后暴露模型「定位强、CWE 方向分类弱」；读/写方向是否计入 scenario 家族匹配属 design 冻结项修订议题，需主会话讨论。报告 `reports/baseline-llm-deepseek-chat.md` 已按轮次重写；llm-eval.yml 补 jsonschema 依赖（该 run CI 评分步骤曾因此失败）
  - 工具（ci.yml run 33737452405 全绿，新 fail-open+钉版首跑）：CSA singletu/CTU、clang-tidy、Infer recall 仍 0（与老基线一致）；CppCheck 16 findings 全 EXTRA（.cpp 收录后 +2）；CodeQL 3 例产出（1 裸 FP）；Joern adapter 修复后见效——defect recall 1/14（r05）+ 4 裸 FP（r04/r09/r14 twin 受保护点全报）；CodeChecker 4 例产出（r07 anchor 正确、r09 报在豁免点属契约违反倾向）；**KLEE v3.2 下 11 例全零 findings 且每例 <1s，系静默失败非真实零发现，runner 诊断修复中**
  - 待办：KLEE runner 修复后重跑该 job；工具报告 v3 整理；第三方标注审计（内审已完成）
- **判定语义修订一项**（2026-09-03 晚，主会话批准；design-v0.4 冻结文档未改，以此为准）：eval.py scenario 家族匹配将 **cwe-125（越界读）/cwe-787（越界写）归并同一「内存越界」族**——读写方向误差不再判 FN，仍体现在 severity/scenario 精确度维度。依据：LLM 轮次二 10 个 FN 中 5 例定位完全正确仅方向报错；KLEE 不区分读写。selftest 已加 4 组防过度扩张断言（cwe-190 vs cwe-125 仍不命中）
- **改造一轮**（2026-09-03 晚）：①上述家族合并；②r11 golden 场景 `build`→`cwe-125`（锚定代码本体越界读，STAGED_TREE 叙事降级为背景）；③codechecker/joern adapter 补 function（joern scan.sc 输出 method.name；codechecker 花括号深度法回推——CodeChecker 6.28.3 JSON 导出确认不含函数上下文），mock 验证 twin 凭 function 翻正 TP。**新口径基线：KLEE 11/11（100%）、LLM 26/30（86.7%）、均 0 误报**；其余工具不变
- 待办：codechecker/joern 的 function 增强要等下次 CI 重跑才体现到基线（本轮归档 findings 无 function）；scan.sc Scala 改动未经实跑待 CI 确认；第三方标注审计
