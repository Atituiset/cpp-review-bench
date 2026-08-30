# cpp-review-bench

**A dual-track benchmark for C/C++ code review: measure both what your reviewer catches (defect track) and what it wrongly flags (contract track).**

> 军规第一原则：本仓一切用例都是**真实、可编译**的 C/C++ 代码——不是题目描述、不是伪代码、不是 markdown 片段。场景的安全性/缺陷性必须在代码本体中可核验。

## 双轨定义

| | Contract Track（契约抑制轨） | Defect Track（缺陷检出轨） |
|---|---|---|
| 回答的问题 | 不该报的别报（FP suppression） | 该报的全报（TP detection） |
| 用例性质 | 「看着有缺陷、契约上安全」的负例 | 含真实缺陷的正例 |
| 核心指标 | FP 数、契约遵守率 | Recall、严重度分级正确率 |
| 防摆烂设计 | 混入 `must_find` 正例探「豁免过度」 | 混入 `must_not_find` 负例探「过度敏感」 |

## 消费形态（工具团队自助取用）

所有消费方共用同一输出契约：**归一化 findings（`schema/findings.schema.json`）→ `tools/eval.py` 评分**。

| 消费方 | 形态 | 入口 |
|---|---|---|
| SA / 分析器（本地直跑） | 全量扫描 | `cmake -S . -B build && <your-analyzer> build/compile_commands.json`，或用例级 `cases/<track>/<id>/src/` 直接指给分析器 |
| SA / 分析器（CI 跑） | GitHub Actions | 本仓 `.github/workflows/ci.yml`：checkout → 全量构建 → CSA 单 TU + 原生 CTU → 评分 |
| Agent+LLM 评审 | diff/PR | 用例 src 可物化为 `diff.patch`（空 base → 新增），或直接以源码树消费 |
| 索引/提取工具 | 编译数据库 | 一键 compdb（`context.navmap_expect` 提供提取判定锚点） |

## 快速上手

```bash
# 1) 全量构建 + 统一 compile_commands.json
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON

# 2) 用例自检（golden 过 schema + anchor/file 真实存在）
python3 tools/check_cases.py

# 3) 评测器自检（构造 findings 验证四态判定正确）
python3 tools/eval.py selftest

# 4) 接一个工具：把它的结果转归一化 findings 后评分
#    （以 Clang SA 为例，本地直跑）
./sa/runners/run_csa.sh singletu /tmp/csa_singletu   # 单 TU 默认
./sa/runners/run_csa.sh ctu      /tmp/csa_ctu        # 原生 CTU（需 clang-extdef-mapping）
python3 tools/eval.py run /tmp/csa_singletu
```

## 目录结构

```
cases/        contract/(16 例) + defect/(14 例) + calibration/
schema/       golden.schema.json(v2) + findings.schema.json
cmake/        AllCases.cmake 一键全量构建 + 统一 compdb
tools/        eval.py(评分器) + check_cases.py(自检) + 其余 *_to_findings.py 已归入 sa/adapters
sa/           静态分析工程化归一目录：
              adapters/   各工具 findings 归一化（*_to_findings.py）
              runners/    各工具触发脚本（run_*.sh）
              scripts/    工具专属查询（joern/scan.sc）
              harnesses/  KLEE 符号执行入口（按 case 归置 klee_harness.c）
              docker/     工具镜像（cooddy/Dockerfile）
reports/      各工具基线报告（持续积累，格式见 reports/README.md）
```

## 评分口径（两层匹配，详见 docs/design-v0.4.md §4）

- **L1 规则匹配（确定性）**：`scenario 家族匹配 + file 精确 + (anchor 去空白子串 或 line±tolerance) + function 精确`。
- **L2 语义判等（可选）**：`rationale` 与工具输出理由做轻量 judge。
- **四态**：PASS / FN（漏报）/ FP（裸 FP 或契约违反）/ EXTRA（多余 finding）。
- **附加维度**：`verified`（编译/复现验证）、`severity` 分级正确率、契约违反（注入 contract.yaml 后仍报 must_not_find，权重 > 裸 FP）。

## 当前状态（建成线 §7.1 进度）

- [x] 前 3 例（c01/c08 contract + r10 defect）五文件齐备、编译通过、过 schema 校验
- [x] `tools/eval.py` 两层匹配 + 四态 + 汇总 + 构造 findings 自检通过
- [x] `cmake/AllCases.cmake` 一键全量构建 + 统一 compdb
- [x] CSA 基线（单 TU 默认 + 原生 CTU）经 CI 跑通
- [ ] 30 例全部铺满
- [ ] 标注审计、≥2 工具基线、Martian 兼容报表

## 公信力争议点

「标注主观性」是社区对 review benchmark 的第一质疑。本仓对策：**判定语义（§3.3）、golden 评审记录、标注审计报告全部随仓公开**；所有 baseline 数字一律标注「自测口径 + 工具版本」，欢迎第三方复测 PR（见 reports/）。

## License

Apache-2.0（详见 LICENSE）。引用方式见 CITATION.cff。
