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

详见 `docs/design-v0.1.md` 与 `docs/roadmap-pr-mining-pipeline.md`。
