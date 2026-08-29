# 基线报告 v2 — 四工具交叉验证（2026-08-29）

本 bench 的核心命题（见 design-v0.4 §2）：**静态分析工具对「外部约束不可证」型
缺陷（长度/索引来自不可信输入、契约守护的安全点）普遍漏报**。本批用四个主流、
技术路线各异的开源 SA 工具在 CI 上对全部 30 例做交叉验证，确认该命题。

## 工具与技术路线

| 工具 | 路线 | 接入方式 | 本次状态 |
|---|---|---|---|
| Clang Static Analyzer (CSA) | 符号执行 | 系统 clang-21（apt 源） | 单 TU + 原生 CTU 均跑通 |
| CppCheck | AST 数据流 | apt 直装 | 跑通，过滤 style 噪声 |
| clang-tidy | AST 风格检查 | clang-21 自带 | 跑通（`-checks=bugprone-*,clang-analyzer-*`） |
| Infer | 抽象解释 | srz-zumix/setup-infer（官方预编译） | 跑通 |
| CodeQL | Datalog DSL（GitHub 原生） | github/codeql-action@v3 | 跑通（v3 将于 2026-12 弃用，后续升 v4） |
| Cooddy | 数据流+约束求解 | 自 build 镜像（GHCR） | **降级**：17 次 build 卡 z3 链接（Solver target `-lz3`），根因见 reports/evidence/cooddy/README.md |

## 评测结果（CI 真跑，本地重算一致）

每个工具对 30 例的评测（eval.py L1 协议：file + anchor 匹配；CWE 工具 scenario=null 时不强制）：

| 工具 | 总 findings | contract recall | defect recall | bare_fp | contract_violation | 有 findings 的 case |
|---|---|---|---|---|---|---|
| CSA 单TU | 0 | 0/16 | 0/14 | 0 | 0 | 无 |
| CSA CTU | 0 | 0/16 | 0/14 | 0 | 0 | 无 |
| CppCheck | 14 | 0/16 | 0/14 | 0 | 0 | c03(5) c02(6) c12(1) c01(1) c01b(1) 全 EXTRA |
| clang-tidy | 0 | 0/16 | 0/14 | 0 | 0 | 无 |
| Infer | 2 | 0/16 | 0/14 | 0 | 0 | c02(2) DEAD_STORE，记 EXTRA |
| CodeQL | 2 | 0/16 | 0/14 | 0 | 0 | r01(`cpp/constant-comparison` 恒真比较) r03(`cpp/unused-static`) 记 EXTRA |

（注：EXTRA = 工具产出但未被 golden 的 must_find/must_not_find 吸收的多余 finding；
非契约违反——因工具报告文本/ruleId 与 golden 锚点子串或 scenario 不匹配，L1 仅记 EXTRA。
这不影响「recall=0 for must_find」结论，但揭示：工具能发现问题点，但其「位置/表述」与
人工标注的 golden 锚点常不一致——正是 Agent Viewer 要解决的「语义对齐」问题。）

## 核心结论

1. **CSA / CppCheck / clang-tidy / Infer 四个工具对全部 30 例的 must_find 真缺陷
   recall 均为 0**。即：传统 SA 对「长度/索引来自不可信输入、编译期约束不可证」的
   越界/空指针/回绕缺陷，全部漏报。这强力印证 design 的核心命题——也是 Agent Viewer
   要补的能力缺口。

2. **CodeQL 是首个对部分缺陷有产出的工具**（r01 恒真比较、r03 未用静态变量），但
   其发现点与 golden 标注的 must_find 锚点不完全重合（报的是同函数内相邻/相关但不同的
   语句），故 eval 记 EXTRA 而非 must_find 命中——说明 CodeQL 有检出能力，但需更细的
   scenario+anchor 对齐（已在 sarif_to_findings.py 加 ruleId→CWE 映射，持续增强中）。

3. **FP 抑制**：contract 轨 must_not_find 安全点（契约守护的判空/越界/死存储占位），
   四工具均未精确误报为「契约违反」（bare_fp/contract_violation 均 0）。但 CppCheck 在
   c02/c03 产生 style 噪声（记 EXTRA）、Infer 在 c02 报 2 个 DEAD_STORE（`last` 审计占位，
   记 EXTRA）、CodeQL 在 r03 报 unused-static（无关噪声，记 EXTRA）——反映传统 SA 在
   contract 安全点上的误报倾向，正是 contract track 要测的 FP 抑制场景。

4. **Cooddy 的价值待补**：Cooddy 自带 CWE 原生映射（cwe-190/125/476），是唯一能让 eval
   走 L1 scenario 家族精确匹配的工具，本应补上「有 scenario 的工具」对照维度。但其 z3
   链接构建障碍未解，暂降级；待 rebuild 或外部镜像后补入。

## 复现

```
# 本地
./consumers/local/run_csa.sh singletu /tmp/csa_singletu
./consumers/local/run_cppcheck.sh /tmp/cppcheck
CLANG_TIDY_BIN=clang-tidy-21 ./consumers/local/run_clang_tidy.sh
# Infer 需容器/setup-infer 环境
python3 tools/eval.py run <findings-dir>   # 四态评测

# CI（golden 路径）
gh workflow run bench-ci.yml   # 跑 CSA/CppCheck/clang-tidy + Infer，产物 all-findings / infer-findings
```

## 证据

- reports/evidence/csa/（singletu + ctu 归一化 findings + 早期归档）
- reports/evidence/cppcheck/（归一化 findings + 基线说明）
- reports/evidence/clang-tidy/（30 例归一化 findings）
- reports/evidence/infer/（30 例归一化 findings）
- reports/evidence/cooddy/README.md（17 次 build 迭代 + z3 链接根因 + 降级说明）
