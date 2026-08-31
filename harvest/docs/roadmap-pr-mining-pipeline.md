# 双管线规划：GitHub PR 自动采集 + bench 评测闭环（单仓版）

> 日期：2026-08-30
> 定位：cpp-review-bench 仓内 `harvest/` 子目录的双管线顶层规划。
> 关系：本文件是 `harvest/docs/design-v0.1.md` 的**姊妹规划**——design-v0.1 走「SA 扫描开源仓源码」路线；
> 本文新增「**爬 GitHub 历史 PR**」采集路线，并把 design-v0.1 的「SA 间共识」验证环升级为
> 「**进仓根 cases/ 用 9 工具实测能否检出**」的硬核评测环（用户核心诉求）。
> 状态：**已实现并跑过多轮**——已并入 cpp-review-bench 单仓（main），pr-mining 采集源每日 cron 在跑
> （matrix 7 仓：curl/sqlite/redis/nginx/vim/postgres/linux）；sa-scan 源为占位未实现，vote 单源退化 min-tools 1。

---

## 1. 目标与双管线定义

用户诉求拆成两条首尾相接的管线，形成**数据飞轮**（同一 GitHub 仓内）：

```
管线 A（采集，harvest/ 负责）
  每日/每周 schedule → 爬 GitHub 开源仓历史 PR（merged + 含 review 讨论）
    → 抽「修复前 diff + 评论里的 bug 指明」→ 候选 bug 单元
    → 归一化 + LLM-as-judge 判真 bug → 写入 harvest/inbox/<draft>/

管线 B（评测，仓根负责）
  inbox 候选 → 转成可编译用例草稿 → 用 9 工具（CSA/CppCheck/clang-tidy/Infer/CodeQL/
    CodeChecker/KLEE/Joern/Cooddy，复用 sa/runners/* + tools/eval.py）跑
    → 看「这个真实 bug 能否被检出」
    → 能检出 → Defect Track 真 TP；工具报了但人审判非 bug → Contract Track 契约；
      都检不出 → 暴露 bench 盲区（反向驱动新增 scenario）

飞轮：评测结果（哪些 SA 在真实 PR bug 上失明）→ 回流 harvest 的噪声/覆盖画像。
```

**核心价值**：现有 bench 的手工种子用例（S 级）验证「工具对已知模式失明」；新管线用
**真实世界 PR bug** 验证「工具对真实缺陷失明」——后者说服力远高于前者，且天然产出
Contract Track 最稀缺的真实 FP 契约。

---

## 2. 论文对标（用户记忆的「爬 PR」思路，已核实存在）

| 论文/项目 | 采集方式 | 验证方式 | 局限 | 我们的差异化 |
|---|---|---|---|---|
| **BugDetectionBench**（moritzWa, GitHub） | 抓 PR **review comments** → LLM 判是否真 bug → 难度分级 | 无（仅数据集） | 靠评论文本，bug 定位弱；无「能否被工具检出」环 | 我们补「进 9 工具实测」环 |
| **Defects4C**（ICLR 投稿） | 爬 **bug-relevant commits**（commit message 关键词） | 人工验证 + APR 修复 | 关键词召回低；C/C++ 可执行基准稀缺 | 我们走 PR（diff+讨论更完整）+ 评测环 |
| **CloudAEye/c_cpp_benchmark** + **withmartian/code-review-benchmark** | scrape **merged PR** → atomic-candidate → LLM-as-judge → F1 leaderboard | LLM reviewer 互评 | 只评「reviewer 能否抓」，不评「SA 能否抓」 | 我们评 SA/符号执行能否抓（不是 LLM） |
| **我们的方案** | 爬 PR（diff + review 讨论 + 修复提交） | **进 cases/ 用 9 工具实跑评测** | 实现成本中；需 GitHub API 配额治理 | 唯一把「真实 PR bug」与「SA 检出能力」闭环的 |

**结论**：「爬 GitHub PR 建 C/C++ 评测集」是已被验证的思路（3 个项目独立做过），但
**没有一篇把采集到的真实 bug 喂进多工具评测闭环**——这正是我们相对论文的差异化壁垒，
也是仓根现有 9 工具矩阵的最佳用武之地。

---

## 3. 与 design-v0.1 的关系与增强

design-v0.1 已设计「SA 扫描开源仓源码 → SARIF 归一化 → ≥2 工具 vote 共识 → 用例草稿 →
inbox 三态人审」。本文**不推翻它，而是叠加两条增强**：

1. **新增采集源（PR 路线）**：design-v0.1 的采集源是「SA 扫描 checkout 的源码」；新增
   「爬 GitHub 历史 PR」作为第二个候选源。两者共用 normalize → vote → pack → inbox 后半段。
   - PR 路线优势：自带「修复前/后 diff」「reviewer 讨论」，bug 定位与真值比纯 SA 扫描更可靠。
   - PR 路线劣势：需 GitHub API、PR 噪声（风格评论、nitpick）、需切片到单函数可编译用例。

2. **验证环升级（评测闭环）**：design-v0.1 的 vote 是「SA 间共识」（≥2 工具独立命中）。
   升级为「候选进 cases/ 用 9 工具实跑，看能否命中」——把「多 SA 共识」替换为
   「仓根已有评分协议的四态判定（PASS/FN/FP/EXTRA）」，且能暴露 bench 自身盲区
   （候选 bug 9 工具全 FN → 说明该真实缺陷类型当前 SA 集体失明 → 反向驱动新增 scenario）。

**对接契约不变**（设计冻结点）：harvest 产出仍是 inbox 五文件草稿，cases/ 三态流转
（confirm-tp → defect/，confirm-fp → contract/ 必填 contract.yaml，reject → 噪声画像）不变。

---

## 4. 单仓结构方案（用户决策：单仓开发）

用户决策：「整个在一个 GitHub 仓开发」。据此 harvest 作为 `cpp-review-bench` 仓的 `harvest/`
子目录存在（不新建独立仓、不跨仓 PR）。理由：
- harvest 本地此前仅 design 草稿、从未 git init、远端不存在 → 零迁移成本并入现有仓。
- 你本意「一起 monorepo 开发」——单仓最贴合；inbox 是仓内目录，三态流转是本地状态机，
  无需 `HARVEST_TOKEN` 跨仓机制。
- 两仓对接点（inbox 五文件、golden schema）本就同 schema，放一个仓只是目录相邻，无功能损失；
  冻结点靠目录约定（bench 核心在 `cases/`+`schema/`+`tools/eval.py`，harvest 在 `harvest/`）守，不必靠 git 边界。
- CI 一个 workflow 矩阵同时跑「bench 9 工具评测（ci.yml）」和「harvest 采集（harvest.yml）」，天然闭环。

```
cpp-review-bench/                 # 现有仓（main 已含 sa/ + KLEE 11 例 + cooddy r04 命中）
├── cases/  schema/  tools/  sa/  reports/   # bench 评测（不变）
├── harvest/                       # 采集管线子目录（已并入 main）
│   ├── docs/design-v0.1.md
│   ├── docs/roadmap-pr-mining-pipeline.md   # 本文件
│   ├── config/{repos,rules}.yaml
│   ├── tools/{pr_mine,normalize,vote,pack_case}.py   # 已实现（pr-mining 唯一采集源）
│   │      # build_compdb.sh / scan_csa.sh / scan_cppcheck.sh / evidence.py：sa-scan 路线规划项，未实现
│   └── inbox/{draft,rejected}/          # 候选落盘（confirm = 移植重写完成后进 cases/，无 confirmed/）
└── .github/workflows/             # 仓根：ci.yml（9 工具评测）+ harvest.yml（采集，每日 cron，matrix 7 仓）
                                   # + harvest-package.yml（三态流转）+ harvest-review.yml
                                   # + harvest-pr-sarif.yml（if:false 禁用中）+ build-cooddy-image.yml
```

---

## 5. 管线详设（增强后的 harvest 主管线）

```
harvest.yml（matrix: source × repo）
  source 维度：
    - sa-scan    （design-v0.1 原路线：checkout 源码 → CSA+CppCheck 扫描）
    - pr-mining   （新增：爬 GitHub 历史 PR → diff 切片 → LLM judge）

  pr-mining 步骤：
    ① 爬取：GitHub API（search/issues?q=is:pr+is:merged+repo:<target>+label:bug 或
       PR review 评论含 bug 关键词）→ 取 PR diff（files/patches）+ review 讨论
    ② 切片：把修复 diff 的「before」函数段切出，配 CMakeLists 编成可编译单元（enclosing file 原样）
    ③ judge：LLM-as-judge 判「是否真 bug + scenario 家族 + 严重度」（可选第三票，替代纯 SA vote）
    ④ 归一化：→ findings（schema 与 schema/findings.schema.json 同源）
    ⑤ pack：→ harvest/inbox/<auto-<repo>-<hash>/ 五文件草稿

harvest-package.yml（候选打包 + inbox 三态流转辅助）
  对 source 产出统一打包；inbox 三态由人审在仓内流转（confirm-tp/confirm-fp/reject）

仓根评测环（承接 inbox）：
  inbox/draft → 取 src → 跑 sa/runners/* 9 工具 + tools/eval.py 四态：
        命中（≥1 工具 must_find 对齐）→ confirm-tp 建议（Defect Track）
        工具报 FP 但 judge 判非 bug → confirm-fp 建议（Contract Track，contract.yaml 必填）
        全 FN（9 工具都漏）→ 标记「bench 盲区」，回流 scenario 新增候选
    → 人审三态最终确认（双人复核）
```

**关键工程决策**：
- **pinned ref**：PR 候选钉死到目标仓 commit + PR 号，可溯源。
- **失败隔离**：`fail-fast: false`；单仓/单 PR/单工具失败记 failure 跳过。
- **PR 切片可编译**：候选 src 必须「真实可编译 C/C++」（bench 军规），diff before 段配最小 CMakeLists。
- **配额治理**：GitHub API rate limit；schedule 错峰 + 结果缓存（同 PR 不重复爬）。
- **license/法律**：候选 src 来自开源仓，notes.md 记录来源仓+commit+PR，不嵌用户名/邮箱/绝对路径。

---

## 6. 阶段计划

- **M0（建仓+分支）** ✅ 已做：harvest 作为 cpp-review-bench 子目录；分支 `exp/harvest-pipeline`（基于 main c2967af）。
- **M1（PR 爬取骨架）**：`tools/pr_mine.py`（GitHub API 爬 PR diff + review → 切片 → LLM judge 占位）→
  归一化 findings。先在 1 个真实开源仓（curl/sqlite，design-v0.1 §6 已列）workflow_dispatch 跑通。
- **M2（进 bench 评测环）**：仓根写 `sa/runners/run_eval_inbox.sh`（对 inbox 草稿跑 9 工具 + eval.py 四态），
  输出「该真实 PR bug 能否被检出」报表。
- **M3（三态流转 + 飞轮）**：inbox 三态人审打通（confirm-tp/confirm-fp/reject）；
  reject 回流 rules.yaml 噪声画像；confirm-fp 的 contract.yaml 反哺 Contract Track。
- **验收（对齐 design-v0.1 §6）**：单仓 PR 爬取端到端跑通；≥10 条候选带 pinned commit/PR + judge 明细；
  仓内完成 1 次三态流转；映射覆盖率报表。

---

## 7. 风险与缓解

| 风险 | 缓解 |
|---|---|
| GitHub API rate limit（PAT 5000/h） | schedule 错峰 + 结果缓存 + 只爬目标白名单仓 |
| PR 噪声（nitpick/风格评论冒充 bug） | LLM-as-judge + scenario 家族映射；unmapped 进人审桶 |
| diff 切片不可编译 | 切片保 enclosing file 原样 + 最小 CMakeLists；编译失败记 failure 跳过 |
| 重复用例（同一 bug 多 PR / 与 S 级种子雷同） | 候选 hash 去重 + 与 cases/ 比对（check_cases 复用） |
| 法律/license | notes.md 记来源不嵌敏感信息；仅用宽松许可开源仓 |
| 与 design-v0.4 契约冻结冲突 | 只新增 `pr-mining` source，不改 inbox 五文件契约 / golden schema |

---

## 8. 是否启动的建议

**已完成启动并落地（事后记录）**：单仓方案已执行，管线已合入 main 并跑过多轮——`pr_mine.py` 真实爬取（M1）、
`sa/runners/run_eval_inbox.sh` 评测环（M2）、三态流转（M3）均已实现；sa-scan 采集源仍为占位，
vote 暂以 `--min-tools 1` 单源退化运行。

---

## 9. 定位修正轮（2026-08）：线索 + blueprint

**动机——五个缺口**：

1. **编译军规冲突**：bench 军规要求 cases/ 下一切用例真实可编译，而 draft 的 src/ 是 diff 原始切片，
   天然不可直接编译——「draft 即半成品用例、确认即入仓」的旧定位与军规直接冲突。
2. **真值未验证**：judge 启发式只判「像不像 bug」，候选触发条件未经验证，人审缺少可复述的判定依据。
3. **license 混染**：候选 src 来自不同许可的源仓（MIT/BSD 与 GPL/RSALv2/Vim 混杂），直接移植有
   copyleft 污染风险，此前无 license 策略。
4. **contract 轨无来源**：defect 候选有 pr-mining 矿，contract 轨（must_not_find）没有对应的采集来源。
5. **分布偏斜**：首跑 76 条候选 72 条来自 nginx，单一仓单一缺陷类型刷屏，无配额约束。

**改造内容（已落地）**：

- **新定位**：harvest = 候选线索生产线 + 人审移植流水线。draft = 线索 + 移植 blueprint，不是半成品用例；
  accept（confirm-tp/contract）= 承诺参照真实案例移植重写一个可编译用例（军规：真实可编译、自然风格、
  无播种标记）后入 cases/，不再是「确认即入仓」。
- **license 策略**：候选 JSON 新增 `license`/`port`/`track_hint`/`polarity` 四顶层字段
  （pr_mine 产出、vote 透传、pack_case 消费；旧候选默认 unknown/rewrite/defect/must_find）。
  direct 组（curl=MIT、sqlite=Public-Domain、nginx=BSD-2-Clause、postgres=PostgreSQL）允许直接移植；
  rewrite 组（redis=RSALv2、vim=Vim、linux=GPL-2.0）只允许参考重写（保留语义、重写表达）。
  license/port/源 PR 只进 notes.md 溯源表，不进 golden.json（golden schema context 为
  additionalProperties:false，冻结不动）。
- **场景配额**：repos.yaml `pr_mining.max_per_scenario`（默认 5，CLI `--max-per-scenario`，
  兜底链 CLI > config > 5）；同一 scenario 桶满后跳过。
- **fp-mining（contract 轨误报矿）**：repos.yaml `pr_mining.fp_mining`（默认 false）+ CLI `--fp-mining`
  显式开启；每仓缺陷轮之后第二轮用 target 的 `fp_query` 抓「修静态分析误报」的 PR（GitHub search
  最多 5 个 OR 算子），不跑 judge_bug，产出 track_hint=contract + polarity=must_not_find 候选，
  scenario 从 PR 标题/diff 关键词映射（映射不了省略）。
- **blueprint notes**：pack_case.py 的 notes.md 改为移植 blueprint 六段——溯源表、缺陷描述与触发条件
  （含「移植者须知：accept 前必须能复述触发条件」）、真实修复 diff、移植要点（外部符号启发式 +
  src/ 不可直接编译声明 + `// <<< BUG ANCHOR` 移植时必须删除 + golden anchor 改用重写后真实代码行）、
  为什么契约安全（仅 contract 候选）、accept 检查清单六项。contract 候选 golden 写 must_not_find 骨架。
- **四态判定口径修正**：`make_draft_sarif.sh` 从 scenario 数字 grep（必不命中）改为 file+anchor 口径
  （SA 命中行源码文本去空白与 golden anchor 互为子串，与 eval.py L1 一致）。
- **propose PR body**：harvest.yml propose 的 PR body 含 draft 定位说明段、轻验证结论
  （run_eval_inbox 产出的 eval_inbox_report.md）、候选溯源总览、accept 检查清单六项、审核指令。

## §10 少 stub 编译轮（2026-08，策略 1+2：闭包切片 + 可编译性打分）

动机：draft 切片缺仓内类型定义（CURLcode 等），无法编译——若移植全压给人，采集省的钱被人工花回去。

- **策略 1 同文件闭包切片**：`pr_mining.closure`（默认 true，CLI `--no-closure`）。拉 base commit
  完整 before 文件，切片引用的同文件定义（static 函数/typedef/struct/#define）递归带到不动点；
  按切片用到的 libc 符号补标准头 include 前导。已知局限：只拉同文件，仓内头文件的类型仍计外部
  依赖（策略 3「仓内头文件按 include 链补一层」未做，视人审成本再定）。
- **策略 2 可编译性打分**：候选新增 `dep_count`（启发式外部符号数，含函数/宏/类型）与
  `compile_errors`（gcc/cc `-fsyntax-only` 实测，权威「编译地板」）；pack_case 按
  compile_errors → dep_count 升序打包，🟢 零依赖以 compile_errors==0 判定。
- **实测（curl 10 PR）**：4 条候选 gcc 错误 ≤3（低 stub 成本优先移植），重依赖候选（如
  conncache.c=57 错）标记「只做 PR/diff 形态评审」。
- 教训：初版 dep_count 只数函数+宏漏了类型（dep=1 实测 11 编译错误），🟢 标签名不副实——
  启发式数字必须有实测信号（compile_errors）兜底才敢给人看。
