# cpp-review-harvest（采集管线，单仓子目录）

本目录是 `cpp-review-bench` 仓的**自动化数据入口**：爬 GitHub 开源仓历史 PR + SA 扫描，
产出可编译的真实 C/C++ 缺陷候选，经人审三态流转进入仓根 `cases/`（双轨）。

与仓根评测构成**双管线闭环**：候选进 `cases/` 后由 `sa/runners/*` + `tools/eval.py`
跑 9 工具，看「真实 bug 能否被检出」——暴露 bench 盲区，反哺 scenario 增长。

## 结构

```
harvest/
├── docs/design-v0.1.md                  # 管线设计（单仓版）
├── docs/roadmap-pr-mining-pipeline.md   # 双管线规划（论文对标 + 评测闭环）
├── config/{repos,rules}.yaml            # 目标仓矩阵 + vote 规则
├── tools/
│   ├── pr_mine.py                       # 爬 GitHub PR diff + 切片 + judge（新增采集源）
│   ├── build_compdb.sh scan_csa.sh scan_cppcheck.sh   # sa-scan 路线（v0.2）
│   ├── normalize.py vote.py pack_case.py             # 归一化/共识/打包
├── .github/workflows/{harvest,package}.yml
└── inbox/{draft,confirmed,rejected}/    # 候选三态
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

- `harvest.yml`：matrix(source×repo) schedule + dispatch；pr-mining 跑通，sa-scan 占位（v0.2）
- `package.yml`：inbox 三态流转（list/confirm-tp/confirm-fp/reject）

详见 `docs/design-v0.1.md` 与 `docs/roadmap-pr-mining-pipeline.md`。
