# cpp-review-harvest 设计文档（v0.1，单仓版）

> 日期：2026-08-30
> 定位：**候选线索生产线 + 人审移植流水线**，作为 `cpp-review-bench` 仓内的 `harvest/` 子目录存在。
> 与 cpp-review-bench 的关系：本目录是「采集管线」，仓根的双轨用例（`cases/`）是「评测目标」；
> 两者靠 `harvest/inbox/` → `cases/` 的人审移植流转对接（同仓内目录，无跨仓 token）。
> draft 不是半成品用例，是「线索 + 移植 blueprint」；accept = 承诺参照真实案例移植重写
> 一个可编译用例（军规：cases/ 下一切用例必须真实可编译、自然风格、无播种标记）。
> 与 gen-auto（`vul-auto-private/gen-auto`）的关系：借鉴其多 SA + SARIF 归一化 + 投票共识机制，
> 但重做三点——**CI 原生（矩阵化 schedule）、双轨产出（defect + contract）、用例级打包**。

---

## 1. 定位与总览

```
每日 schedule（GitHub Actions，本仓 .github/workflows/harvest.yml，cron 23 3 * * *）
  ┌─ 采集源矩阵（config/repos.yaml；pr-mining 爬 PR 已实现，sa-scan 源码扫描为占位未实现）
  │    ① pr-mining：GitHub API 爬历史 PR diff+review → 切片 → judge 启发式
  │      （fp-mining 可选第二轮：抓「修静态分析误报」的 PR → contract 轨 must_not_find 候选）
  │      （sa-scan 规划路线：检出 + 生成 compile_commands.json → Clang SA + CppCheck 扫描）
  │    ② SARIF 归一化 → 统一 findings（schema 与仓根 schema/findings.schema.json 同源）
  │    ③ vote：≥2 工具独立命中 → 缺陷候选（当前单源退化 --min-tools 1，待 sa-scan 上线恢复）
  │    ④ 候选打包：diff 上下文切片 + golden.json 草稿 + notes.md 移植 blueprint
  │    ⑤ 写入本仓 harvest/inbox/draft/<case-draft>/
  ▼
人审移植流水线（仓内，cases/ 治理）
  ├─ confirm-tp → 承诺移植重写一个可编译用例后入 cases/defect/（军规：真实可编译、自然风格、无播种标记）
  ├─ contract   → 同上移植重写后入 cases/contract/（contract.yaml 必填）
  └─ reject     → harvest/inbox/rejected/（原因必填，回流 rules.yaml 噪声画像）
```

**draft 的定位**：draft 不是半成品用例，是「线索 + 移植 blueprint」——notes.md 给出溯源、触发条件、
真实修复 diff 与移植要点，src/ 是原始切片（不可直接编译，仅作移植参照）。accept 不是「确认即入仓」，
而是承诺参照真实案例移植重写一个满足 bench 军规的可编译用例后再入 `cases/`。

**双管线闭环（用户核心诉求）**：采集到的真实候选进 `cases/` 后，由仓根 `sa/runners/*` + `tools/eval.py`
跑 9 工具评测（CSA/CppCheck/clang-tidy/Infer/CodeQL/CodeChecker/KLEE/Joern/Cooddy），看「该真实 bug
能否被检出」——这是 design-v0.1 原「SA 间共识」验证环的升级（见 `harvest/docs/roadmap-pr-mining-pipeline.md`）。

**双轨产出的关键**：真实世界里工具报告的 finding 天然分两类——真缺陷和「工具自己也会犯的 FP」。
人审判 FP 的候选不是垃圾，是 **Contract Track 最稀缺的真实契约用例**（带「哪个工具在哪误报」完整证据）。
一条管线，两头收货。

## 2. 仓库结构（单仓内 harvest/ 子目录）

```
cpp-review-bench/                # 仓根（不变）
├── cases/  schema/  tools/  sa/  reports/   # bench 评测（不变）
├── .github/workflows/           # 仓根 workflow（harvest 与 bench 共用）
│   ├── harvest.yml              # 主管线（matrix: source × repo），每日 schedule + workflow_dispatch
│   ├── harvest-package.yml      # 候选打包 + inbox 三态流转辅助
│   ├── harvest-review.yml       # 候选复核辅助
│   ├── harvest-pr-sarif.yml     # PR 内联 SARIF（if:false 禁用中，改由 harvest.yml 统一落 code-scanning）
│   └── build-cooddy-image.yml   # cooddy 镜像构建
└── harvest/                       # 本目录
    ├── README.md
    ├── docs/design-v0.1.md        # 本文件
    ├── docs/roadmap-pr-mining-pipeline.md   # 双管线规划报告
    ├── config/
    │   ├── repos.yaml            # 目标仓矩阵：git url、构建方式、扫描范围（7 仓）
    │   └── rules.yaml            # vote 规则、噪声抑制、scenario 键映射
    ├── tools/
    │   ├── pr_mine.py            # 【当前唯一采集源】爬 GitHub 历史 PR diff + review → 切片 → judge 启发式
    │   ├── normalize.py          # 多源 findings → 归一化
    │   ├── vote.py               # 共识：≥2 工具命中 → 候选（当前单源退化 min-tools 1）
    │   ├── pack_case.py          # finding → 用例五文件草稿
    │   └── （build_compdb.sh / scan_csa.sh / scan_cppcheck.sh / evidence.py：sa-scan 路线规划项，未实现）
    └── inbox/                    # 待审核候选（仓内目录，落盘 draft/rejected；confirm = 移植重写完成后进 cases/）
```

## 3. 主管线（harvest.yml）

```yaml
on:
  schedule: [{cron: "23 3 * * *"}]   # 每日凌晨（错峰，避免 API 配额尖峰）
  workflow_dispatch:
    inputs:
      source: {type: choice, options: [all, sa-scan, pr-mining]}
      repo:   {type: choice, options: [all, curl, sqlite, redis, nginx, vim, postgres, linux]}

jobs:
  scan:
    strategy:
      fail-fast: false
      matrix:
        source: [sa-scan, pr-mining]
        repo: [curl, sqlite, redis, nginx, vim, postgres, linux]
    steps:
      - pr-mining: pr_mine.py（GitHub API 爬 PR diff+review）→ 切片 → judge → normalize → artifact
      - sa-scan（占位未实现）: 检出目标仓（pinned ref）→ build_compdb → scan_<tool>.sh → normalize → artifact
  vote:
    needs: scan
    steps:
      - 下载全部 findings artifacts
      - vote.py → candidates.json（当前单源退化 --min-tools 1）
      - pack_case.py → harvest/inbox/draft/<case-draft>/
      - 上传 artifact: candidates-<date>.zip
```

设计要点（与 design-v0.4 军规一致）：
- **矩阵可扩展**：repo × source × tool 多维；新仓 = repos.yaml 加一行；新工具 = matrix 加一列 + scan_xxx.sh。
- **pinned ref**：目标仓 commit / PR 号钉死并写入产物——任何候选可溯源到确切代码版本。
- **失败隔离**：`fail-fast: false`；单仓/单 PR/单工具失败不拖垮整轮，记 failure 跳过。
- **成本形态**：schedule 驱动 + artifact 存储；大仓后续评估自托管 runner。
- **场景配额**：repos.yaml `pr_mining.max_per_scenario`（默认 5，CLI `--max-per-scenario`，
  兜底链 CLI > config > 5）。同一 scenario 桶满后跳过，防单一仓单一缺陷类型刷屏
  （首跑 76 条候选 72 条来自 nginx 的教训）。
- **fp-mining（contract 轨误报矿）**：repos.yaml `pr_mining.fp_mining`（默认 false）+ CLI `--fp-mining`
  显式开启。开启后每仓在缺陷轮之后跑第二轮采集，query 用 target 的 `fp_query`
  （默认 `"false positive" OR "false-positive" OR "intended behavior" OR "not a bug" OR cppcheck OR clang-tidy`，
  注意 GitHub search 最多 5 个 OR 算子），抓「修静态分析误报」的 PR；不跑 judge_bug，
  产出 track_hint=contract + polarity=must_not_find 候选，scenario 从 PR 标题/diff 关键词映射
  （映射不了省略）。
- **propose PR body**：harvest.yml propose 步骤产出的 PR body 含 draft 定位说明段、轻验证结论
  （run_eval_inbox 产出的 eval_inbox_report.md）、候选溯源总览、accept 检查清单六项、审核指令。

## 4. 候选打包（pack_case.py）

对每个共识 finding 自动生成 bench 候选草稿（五文件，与仓根 cases/<id>/ 同构）。
**draft = 线索 + 移植 blueprint，不是半成品用例**：

```
harvest/inbox/<auto-<repo>-<hash>/
├── src/                  # 缺陷函数 + 最小上下文（enclosing file 原样切片，明示不可直接编译，仅作移植参照）
├── CMakeLists.txt        # 复用目标仓构建片段或生成 -c 编译目标
├── golden.json           # 草稿：defect 候选 must_find {scenario(由规则 ID 映射), file, anchor(自动抓语句), function}；
│                         # contract 候选写 must_not_find 骨架
├── contract.yaml         # 空（contract 轨 accept 时填写——这行字就是契约提炼）
└── notes.md              # 移植 blueprint 六段（见下）
```

候选 JSON 新增四个顶层字段（pr_mine.py 产出，vote.py 透传，pack_case.py 消费；旧候选无字段时默认
license=unknown / port=rewrite / track_hint=defect / polarity=must_find）：

- `license`：源仓许可证（curl=MIT、sqlite=Public-Domain、nginx=BSD-2-Clause、postgres=PostgreSQL 为
  direct 组；redis=RSALv2、vim=Vim、linux=GPL-2.0 为 rewrite 组）
- `port`：`direct`（宽松许可，允许直接移植代码）/ `rewrite`（copyleft 或非宽松，只允许参考重写——
  保留语义、重写表达）
- `track_hint`：defect / contract（contract 来自 fp-mining 误报矿）
- `polarity`：must_find / must_not_find

**license/port/源 PR 不进 golden.json**（schema 的 context 是 additionalProperties:false，冻结不动），
只在 notes.md 溯源表。

notes.md 移植 blueprint 六段（pack_case.py 生成）：

1. **溯源表**：源仓 / 源 PR / 许可证 / 移植策略 / 采集时间 / track 方向
2. **缺陷描述与触发条件**：含「移植者须知：accept 前必须能复述触发条件」
3. **真实修复 diff**
4. **移植要点**：外部符号启发式列表；明示 src/ 是原始切片不可直接编译；`// <<< BUG ANCHOR` 标记
   移植时必须删除；golden anchor 改用重写后真实代码行
5. **为什么契约安全**（仅 contract 候选模板）
6. **accept 检查清单六项**：编译通过 / anchor 真实 / 触发条件已复述 / license 策略已遵守 /
   BUG ANCHOR 标记已清除 / notes 三段式已补全

规则 ID → scenario 映射（config/rules.yaml）：CSA checker 名 / CppCheck id → CWE 键
（如 `core.NullDereference→cwe-476`、`doubleFree→cwe-415`）；映射不了的进 `unmapped` 桶由人审归类。

## 5. 人审移植流水线（仓内，cases/ 治理）

候选进入 `harvest/inbox/`（draft 态 = 线索 + 移植 blueprint），按用例状态机流转
（与 design-v0.4 §7.3 同构）：

```
harvest/inbox/<id>/  (draft)
   │  审核人三选一并必填证据（动作名沿用 harvest-package.yml，语义已升级为「移植承诺」）
   ├─ confirm-tp → 承诺参照真实案例移植重写一个可编译用例（军规：真实可编译、自然风格、
   │               无播种标记；port=rewrite 时保留语义、重写表达），完成后入 cases/defect/<id>/；
   │               不再是「确认即入仓」——accept 检查清单六项逐项过
   │              （编译通过 / anchor 真实 / 触发条件已复述 / license 策略已遵守 /
   │                BUG ANCHOR 标记已清除 / notes 三段式已补全）
   ├─ contract   → 同上移植重写后入 cases/contract/<id>/（**必填 contract.yaml**：FP 因何契约
   │               成立，每条 = 一条真实世界 exemption_pattern 入库）
   └─ reject     → harvest/inbox/rejected/<id>/（原因必填：误报规则/重复/无法复现；
                   回流 rules.yaml 噪声黑名单与 SA 规则画像）
```

**轻验证（accept 前的机器辅证）**：harvest.yml propose 步骤跑 run_eval_inbox 产出
eval_inbox_report.md 进 PR body；`make_draft_sarif.sh` 的四态判定已从 scenario 数字 grep
（必不命中）改为 file+anchor 口径（SA 命中行源码文本去空白与 golden anchor 互为子串，
与 eval.py L1 一致）。

治理原则：双人复核（confirm 需第二人签字）；审核 SLA 与积压量进度量；候选标题带来源仓+commit/PR 便于追溯。

## 6. v0.1 范围与验收

**做**：7 个目标仓（curl/sqlite/redis/nginx/vim/postgres/linux，matrix 见仓根 harvest.yml）×
采集源（pr-mining 已实现并跑过多轮；sa-scan 为占位未实现）× vote（单源退化 min-tools 1）+ 打包 + inbox 人审移植。
**不做**：Infer（构建重，v0.2 评估）、自托管 runner、目标仓编译通过保证（失败记 failure 跳过）、自动修 bug。

**验收标准**：
- [ ] `workflow_dispatch` 单仓（aetherstack）pr-mining 端到端跑通：爬 PR → 切片 → judge → 候选
- [ ] 4 仓 + 2 源一轮产出候选 ≥10 条，每条带 pinned commit/PR + judge 明细 + 证据链
- [ ] 仓内完成一次移植流转：1 条 confirm-tp 移植重写后入 defect/、1 条 contract 移植重写后入 contract/（contract.yaml 非空；两者 accept 检查清单六项全过）、1 条 reject 带原因
- [ ] 映射覆盖率报表：规则 ID → scenario 的 mapped/unmapped 分布

## 7. 与现有体系的关系

- **输入**：gen-auto 归一化/投票模式（思路）+ 本仓 `sa/adapters/*_to_findings.py` 的 SARIF 原生管线（schema 复用）
- **输出**：仓根 `cases/` 双轨用例（Defect Track 真实 TP 来源 + Contract Track 真实世界契约）
- **飞轮**：审核数据（哪些 SA 规则 FP 率高）→ rules.yaml 噪声画像 → 采集质量自提升；
  contract 动作的 contract.yaml → exemption_pattern 契约库 → 反哺 Contract Track
- **评测闭环**：候选入 `cases/` 后由仓根 9 工具评测，暴露 bench 盲区（反向驱动新增 scenario）
- **CI 基建**：复用仓根 `.github/workflows/ci.yml` 的 9 工具 job 模式；评测环可直接复用 `sa/runners/`
