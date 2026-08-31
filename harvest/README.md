# cpp-review-harvest（采集管线，单仓子目录）

本目录是 `cpp-review-bench` 仓的**候选线索生产线 + 人审移植流水线**：爬 GitHub 开源仓历史 PR
（+ 可选 fp-mining 误报矿），产出「线索 + 移植 blueprint」形式的 draft 候选；accept = 承诺参照真实案例
移植重写一个真实可编译用例（自然风格、无播种标记），完成后进入仓根 `cases/`（双轨）。

与仓根评测构成**双管线闭环**：候选进 `cases/` 后由 `sa/runners/*` + `tools/eval.py`
跑 9 工具，看「真实 bug 能否被检出」——暴露 bench 盲区，反哺 scenario 增长。

## 结构

```
harvest/
├── docs/design-v0.1.md                  # 管线设计（单仓版）
├── docs/roadmap-pr-mining-pipeline.md   # 双管线规划（论文对标 + 评测闭环）
├── config/{repos,rules}.yaml            # 目标仓矩阵（7 仓）+ vote 规则
├── tools/
│   ├── pr_mine.py                       # 爬 GitHub PR diff + 切片 + judge（当前唯一采集源）
│   ├── normalize.py vote.py pack_case.py             # 归一化/共识（min-tools 1）/打包
│   └── （build_compdb.sh / scan_csa.sh / scan_cppcheck.sh / evidence.py：sa-scan 路线规划项，未实现）
├── inbox/{draft,rejected}/              # 候选落盘两态（confirm = 移植重写完成后进 cases/，无 confirmed/）
└── （workflow 在仓根 .github/workflows/：harvest.yml / harvest-package.yml / harvest-review.yml /
     harvest-pr-sarif.yml（if:false 禁用中）/ build-cooddy-image.yml）
```

## 快速上手（本地冒烟）

```bash
# 单仓爬 PR（需 export GITHUB_TOKEN，公开仓读权限即可）
pip install requests pyyaml
python3 harvest/tools/pr_mine.py --repo curl/curl --max-prs 5 --out /tmp/pr-out
python3 harvest/tools/normalize.py --in-dir /tmp/pr-out --out /tmp/norm.json
python3 harvest/tools/vote.py --findings /tmp/norm.json --out /tmp/cands.json --min-tools 1
python3 harvest/tools/pack_case.py --candidates /tmp/cands.json --inbox harvest/inbox
```

## 使用流程（端到端）

### 1. 采集：自动 or 手动

- **日常**：`harvest.yml` 每日 cron 自动跑（缺陷轮，fp-mining 默认关闭），有候选就开审核 PR。
- **手动**：Actions → harvest → Run workflow，输入说明：

| 输入 | 含义 | 默认 |
|---|---|---|
| `source` | all / sa-scan（占位）/ pr-mining | all |
| `repos` | 逗号分隔仓过滤（curl, sqlite, redis, nginx, vim, postgres, linux），留空=全量 | 空 |
| `since` | 历史批扫起点 YYYY-MM-DD；留空=增量（昨天起） | 空 |
| `max_prs` | 每仓每轮 PR 上限；深历史批扫调大（如 1000） | 50 |
| `max_per_pr` | 每 PR 最多候选数（防大 PR 刷屏） | 3 |
| `max_candidates` | 每仓候选总数上限（0=不限） | 0 |
| `max_per_scenario` | 每 scenario 配额（防单一缺陷类型刷屏） | 5 |
| `fp_mining` | 开启 contract 轨误报矿（缺陷轮后加跑第二轮） | false |

### 2. 看候选 PR

propose 自动开出「harvest: 每日候选 N 条待审核」PR，body 含五段：draft 定位说明 →
轻验证结论（单文件 SA 的 file+anchor 命中报表）→ 候选溯源总览（四态表）→ accept 检查清单 →
审核指令。每条候选在 `harvest/inbox/draft/<id>/` 下五文件，**notes.md 是移植 blueprint**：
溯源表（源仓/源 PR/许可证/移植策略）→ 缺陷描述与触发条件 → 真实修复 diff → 移植要点 →
（contract 候选）为什么契约安全 → accept 检查清单。

**draft 不是用例**：src/ 是原始切片（策略 1 闭包后已带上同文件依赖与标准头），
golden.json 是骨架（缺 id/track/title 等必填字段）。**优先挑低 stub 成本的候选做**：
notes 溯源表有「编译错误数（gcc syntax-only）」与「dep_count」两列，🟢 零依赖候选
（compile_errors=0）开箱即编译，错误数 ≤3 的通常只需补两三个类型声明。

### 3. 移植重写（accept 前必须完成）

在候选分支上操作（`git checkout harvest/inbox-<date>-<run_id>`），对每条要收的候选：

1. 读 notes.md，**用一句话复述触发条件**并写进 notes
2. 参照真实案例**重写 src/ 为可独立编译的代码**（自然风格、无播种痕迹）；
   `port=rewrite` 的仓（redis/vim/linux）只许保留语义、重写表达，不得照抄切片
3. 删除 `// <<< BUG ANCHOR` 标记；golden.json 补全 schema 必填字段，
   **anchor 改用重写后真实存在的代码行**
4. contract 候选：填 `contract.yaml`（FP 因何契约成立）+ notes「为什么契约安全」段
5. 本地验证后 push 回候选分支：

```bash
python3 tools/check_cases.py     # golden 过 schema + anchor/file 真实存在
cmake -S . -B build && cmake --build build   # 全量编译通过
```

### 4. 评论指令流转（在 PR 评论区执行，非本地）

指令由 `harvest-review.yml`（GitHub Actions，issue_comment 触发）在**候选分支**上执行
git mv 并回帖结果；一行一条，空格分隔多个 id：

```
/case accept   auto-curl-aaa auto-curl-bbb    → git mv 到 cases/defect/<id>/
/case contract auto-curl-ccc                  → git mv 到 cases/contract/<id>/（contract.yaml 非空才执行，否则跳过）
/case reject   auto-curl-ddd --reason "误报：非内存安全"   → git mv 到 harvest/inbox/rejected/<id>/ + REJECT_REASON.md
```

注意：**流转是机械 git mv，不校验移植是否完成**——所以第 3 步必须先做，
否则骨架 case 进 cases/ 后 `check_cases.py` 挂、merge 后 main CI 变红。
track 方向别搞反：fp 矿候选（notes 溯源表标「contract 候选」）用 `/case contract`，
用 `/case accept` 会错误进入 defect 轨。

### 5. merge 落地

候选 PR merge 进 main 后，入 cases/ 的用例由 `ci.yml` 跑 9 工具四态（eval.py），
rejected/ 候选的原因回流 rules.yaml 噪声黑名单。

## CI

- `.github/workflows/harvest.yml`：matrix(source × 7 仓) 每日 schedule + dispatch；pr-mining 已实现并跑过多轮，sa-scan 占位未实现；vote 单源退化 `--min-tools 1`；propose 步骤产出含 draft 定位说明、轻验证结论（eval_inbox_report.md）、候选溯源总览与 accept 检查清单的 PR
- `.github/workflows/harvest-package.yml`：inbox 流转辅助（list/confirm-tp/contract/reject；confirm 不再是「确认即入仓」，而是承诺参照真实案例移植重写一个可编译用例后入 cases/）
- 另有 `harvest-review.yml`、`harvest-pr-sarif.yml`（if:false 禁用中）、`build-cooddy-image.yml`

## 本轮改造完成项（2026-08，定位修正：线索 + blueprint）

- **license 策略**：候选 JSON 新增 `license`/`port`/`track_hint`/`polarity` 四个顶层字段；direct 组（curl/sqlite/nginx/postgres）允许直接移植，rewrite 组（redis/vim/linux）只允许参考重写；license/port/源 PR 只进 notes.md 溯源表，不进 golden.json
- **场景配额**：`pr_mining.max_per_scenario`（默认 5，CLI `--max-per-scenario`），同 scenario 桶满即跳过，防单一仓单一缺陷类型刷屏
- **fp-mining**：`pr_mining.fp_mining`（默认 false）+ CLI `--fp-mining`，每仓缺陷轮之后第二轮抓「修静态分析误报」PR，产出 contract 轨 must_not_find 候选
- **blueprint notes**：pack_case.py 的 notes.md 改为移植 blueprint 六段（溯源表/缺陷描述与触发条件/真实修复 diff/移植要点/为什么契约安全/accept 检查清单六项）
- **四态判定口径修正**：`make_draft_sarif.sh` 从 scenario 数字 grep（必不命中）改为 file+anchor 口径，与 eval.py L1 一致

## 本轮改造完成项（2026-08，少用/不用 stub 即可编译：策略 1+2）

- **策略 1 同文件闭包切片**（默认开，`pr_mining.closure` / CLI `--no-closure`）：拉 base commit 完整文件，把切片引用的同文件定义（static 函数/typedef/struct/#define）递归带上——同仓真实代码，零 stub；另按切片用到的 libc 符号补标准头 include 前导（自然 C 代码，非 stub）
- **策略 2 可编译性打分与优先**：候选带两个信号——`dep_count`（启发式外部符号数，含函数/宏/类型）与 `compile_errors`（gcc/cc `-fsyntax-only` 实测错误数，权威「编译地板」，无编译器时省略）；pack_case 按 compile_errors → dep_count 升序打包，🟢 零依赖标记以 compile_errors==0 优先判定，notes 溯源表两列齐出，四态表带 dep 列
- 实测口径（curl 10 PR）：4 条候选 gcc 错误 ≤3（低 stub 成本，优先移植），重依赖候选（如 digest.c=12 错）降级——curl 系仓的类型依赖多住在仓内头文件，如需进一步降 stub 再走策略 3（仓内头文件补一层，未做）

详见 `docs/design-v0.1.md` 与 `docs/roadmap-pr-mining-pipeline.md`。
