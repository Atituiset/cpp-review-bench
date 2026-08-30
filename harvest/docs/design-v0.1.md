# cpp-review-harvest 设计文档（v0.1，单仓版）

> 日期：2026-08-30
> 定位：**benchmark 的自动化数据入口**，作为 `cpp-review-bench` 仓内的 `harvest/` 子目录存在。
> 与 cpp-review-bench 的关系：本目录是「采集管线」，仓根的双轨用例（`cases/`）是「评测目标」；
> 两者靠 `harvest/inbox/` → `cases/` 的三态流转对接（同仓内目录，无跨仓 token）。
> 与 gen-auto（`vul-auto-private/gen-auto`）的关系：借鉴其多 SA + SARIF 归一化 + 投票共识机制，
> 但重做三点——**CI 原生（矩阵化 schedule）、双轨产出（defect + contract）、用例级打包**。

---

## 1. 定位与总览

```
每周/每晚 schedule（GitHub Actions，本仓 .github/workflows/harvest.yml）
  ┌─ 采集源矩阵（config/repos.yaml + 本仓设计：sa-scan 源码扫描 / pr-mining 爬 PR）
  │    ① 检出 + 生成 compile_commands.json（cmake / make+compiledb / bear）
  │    ② 多工具扫描：Clang SA + CppCheck（+可选：场景评审 LLM）
  │    ③ SARIF 归一化 → 统一 findings（schema 与仓根 schema/findings.schema.json 同源）
  │    ④ vote：≥2 工具独立命中 → 缺陷候选；codegraph 调用链证据附挂
  │    ⑤ 用例打包：diff 上下文切片 + golden.json 草稿 + notes 骨架
  │    ⑥ 写入本仓 harvest/inbox/<case-draft>/（三态：draft/confirmed/rejected）
  ▼
人工审核管线（仓内，cases/ 治理）
  ├─ 确认 TP → cases/defect/ 转正（Defect Track 增量）
  ├─ 判定 FP（契约安全）→ cases/contract/ 转正（Contract Track 增量，真实世界契约案例）
  └─ 驳回 → harvest/inbox/rejected/（原因必填，回流 rules.yaml 噪声画像）
```

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
└── harvest/                       # 本目录
    ├── README.md
    ├── docs/design-v0.1.md        # 本文件
    ├── docs/roadmap-pr-mining-pipeline.md   # 双管线规划报告
    ├── config/
    │   ├── repos.yaml            # 目标仓矩阵：git url、构建方式、扫描范围
    │   └── rules.yaml            # vote 规则、噪声抑制、scenario 键映射
    ├── .github/workflows/
    │   ├── harvest.yml           # 主管线（matrix: source × repo），schedule + workflow_dispatch
    │   └── package.yml           # 候选打包 + inbox 三态流转辅助
    ├── tools/
    │   ├── pr_mine.py            # 【新增采集源】爬 GitHub 历史 PR diff + review → 切片 → LLM judge
    │   ├── build_compdb.sh       # 各构建系统 compdb 适配
    │   ├── scan_csa.sh           # clang --analyze 包装（SARIF）
    │   ├── scan_cppcheck.sh      # cppcheck --xml → SARIF
    │   ├── normalize.py          # 多源 SARIF → 归一化 findings
    │   ├── vote.py               # 共识：≥2 工具命中 → 候选
    │   ├── evidence.py           # codegraph 调用链证据提取
    │   └── pack_case.py          # finding → 用例五文件草稿
    └── inbox/                    # 待审核候选（仓内目录，三态：draft/confirmed/rejected）
```

## 3. 主管线（harvest.yml）

```yaml
on:
  schedule: [{cron: "23 3 * * 1,4"}]   # 周一/周四凌晨
  workflow_dispatch:
    inputs:
      source: {type: choice, options: [all, sa-scan, pr-mining]}
      repo:   {type: choice, options: [all, curl, sqlite, redis, aetherstack]}

jobs:
  scan:
    strategy:
      fail-fast: false
      matrix:
        source: [sa-scan, pr-mining]
        repo: [curl, sqlite, redis, aetherstack]
    steps:
      - sa-scan: 检出目标仓（pinned ref）→ build_compdb → scan_<tool>.sh → normalize → artifact
      - pr-mining: pr_mine.py（GitHub API 爬 PR diff+review）→ 切片 → LLM judge → normalize → artifact
  vote:
    needs: scan
    steps:
      - 下载全部 findings artifacts
      - vote.py → candidates.json
      - evidence.py（codegraph 链回溯附挂）
      - pack_case.py → harvest/inbox/<case-draft>/
      - 上传 artifact: candidates-<date>.zip
```

设计要点（与 design-v0.4 军规一致）：
- **矩阵可扩展**：repo × source × tool 多维；新仓 = repos.yaml 加一行；新工具 = matrix 加一列 + scan_xxx.sh。
- **pinned ref**：目标仓 commit / PR 号钉死并写入产物——任何候选可溯源到确切代码版本。
- **失败隔离**：`fail-fast: false`；单仓/单 PR/单工具失败不拖垮整轮，记 failure 跳过。
- **成本形态**：schedule 驱动 + artifact 存储；大仓后续评估自托管 runner。

## 4. 候选打包（pack_case.py）

对每个共识 finding 自动生成 bench 用例草稿（五文件，与仓根 cases/<id>/ 同构）：

```
harvest/inbox/<auto-<repo>-<hash>/
├── src/                  # 缺陷函数 + 最小上下文（enclosing file 原样，不做切片改写）
├── CMakeLists.txt        # 复用目标仓构建片段或生成 -c 编译目标
├── golden.json           # 草稿：must_find {scenario(由规则 ID 映射), file, anchor(自动抓语句), function}
│                         # must_not_find: []（留人审补充）
├── contract.yaml         # 空（人审判 FP 时填写——这行字就是契约提炼）
└── notes.md              # 骨架：来源仓+commit/PR、命中工具、vote 明细、codegraph 证据链
```

规则 ID → scenario 映射（config/rules.yaml）：CSA checker 名 / CppCheck id → CWE 键
（如 `core.NullDereference→cwe-476`、`doubleFree→cwe-415`）；映射不了的进 `unmapped` 桶由人审归类。

## 5. 人工审核管线（仓内，cases/ 治理）

候选进入 `harvest/inbox/`（draft 态），按用例状态机流转（与 design-v0.4 §7.3 同构）：

```
harvest/inbox/<id>/  (draft)
   │  审核人三选一并必填证据
   ├─ confirm-tp    → cases/defect/<id>/（golden 补 anchor/rationale/severity，锚点复核）
   ├─ confirm-fp    → cases/contract/<id>/（**必填 contract.yaml**：FP 因何契约成立，
   │                  每条 FP 确认 = 一条真实世界 exemption_pattern 入库）
   └─ reject        → harvest/inbox/rejected/<id>/（原因必填：误报规则/重复/无法复现；
                      回流 rules.yaml 噪声黑名单与 SA 规则画像）
```

治理原则：双人复核（confirm 需第二人签字）；审核 SLA 与积压量进度量；候选标题带来源仓+commit/PR 便于追溯。

## 6. v0.1 范围与验收

**做**：4 个目标仓（aetherstack smoke + curl/sqlite/redis）× 2 采集源（sa-scan + pr-mining）×
vote + 打包 + inbox 三态审核。pr-mining 优先在 1 个真实仓 workflow_dispatch 跑通。
**不做**：Infer（构建重，v0.2 评估）、自托管 runner、目标仓编译通过保证（失败记 failure 跳过）、自动修 bug。

**验收标准**：
- [ ] `workflow_dispatch` 单仓（aetherstack）pr-mining 端到端跑通：爬 PR → 切片 → judge → 候选
- [ ] 4 仓 + 2 源一轮产出候选 ≥10 条，每条带 pinned commit/PR + judge 明细 + 证据链
- [ ] 仓内完成一次三态流转：1 条 confirm-tp 入 defect/、1 条 confirm-fp 入 contract/（contract.yaml 非空）、1 条 reject 带原因
- [ ] 映射覆盖率报表：规则 ID → scenario 的 mapped/unmapped 分布

## 7. 与现有体系的关系

- **输入**：gen-auto 归一化/投票模式（思路）+ 本仓 `sa/adapters/*_to_findings.py` 的 SARIF 原生管线（schema 复用）
- **输出**：仓根 `cases/` 双轨用例（Defect Track 真实 TP 来源 + Contract Track 真实世界契约）
- **飞轮**：审核数据（哪些 SA 规则 FP 率高）→ rules.yaml 噪声画像 → 采集质量自提升；
  confirm-fp 的 contract.yaml → exemption_pattern 契约库 → 反哺 Contract Track
- **评测闭环**：候选入 `cases/` 后由仓根 9 工具评测，暴露 bench 盲区（反向驱动新增 scenario）
- **CI 基建**：复用仓根 `.github/workflows/ci.yml` 的 9 工具 job 模式；评测环可直接复用 `sa/runners/`
