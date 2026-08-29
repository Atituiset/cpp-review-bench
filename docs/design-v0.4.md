# cpp-review-bench 设计文档（v0.4 · 双轨社区版）

> 日期：2026-08-29
> 定位：**面向 C/C++（含混合编程）的代码评审基准**。双轨设计——
> **契约抑制轨（Contract Track）**：测「不该报的别报」（FP suppression）；
> **缺陷检出轨（Defect Track）**：测「该报的全报」（TP detection）。
> 与任何检视工具（SA / LLM 评审）、任何上下文工具（navmap / codegraph / clangd / 自研 CSA）解耦。
> 目标：成为 C/C++ review 工具评估的社区基准之一。

---

## 0. 与既有工作的关系

- 场景来源：`agent-reviewer/docs/design/high-fp-scenarios.md`（C/C++/混合编程/IPC 四篇 70 条误报地图）
- 评测协议与 golden 规范：参考《C/C++ Code Review 评测集建设：深度调研报告》（OCR 仓 `papers/cpp-review-benchmark-research.md`）——golden comment schema（CWE tag / severity / 语句级锚点 / function / rationale）、两层匹配协议（规则匹配 + 轻量 judge）、per-CWE 分层报表、防泄漏切分、hard case 子集、标注审计（每类抽检复核）。本文将其「真缺陷 PR 轨」作为 Defect Track 的二期来源，一期用合成 TP 先行
- 真实缺陷采集管线：参考 `vul-auto-private/gen-auto`——多静态分析器（Infer / Clang SA / CppCheck）扫描开源仓 → SARIF 归一化（`parse_sarif.py` / `materialize.py`）→ 多工具投票共识（`vote.py`）→ 人审入 golden。该管线是 Defect Track 真实 TP 的低成本来源（见 §5.3）
- 对标：[Martian Code Review Bench](https://codereview.withmartian.com/)（50 PR，社区新兴的 AI 评审工具榜）——v1.1 起输出 Martian 兼容报告，便于与既有 PR 级评测交叉对照；同时吸取其第三方复测暴露的四条教训（版本一致性 / 子集漂移 / 标注口径 / recall 粒度），对应设计见 §4.2
- 差异化定位：现有 C/C++ 漏洞检测集（PrimeVul/DiverseVul/CASTLE 等）全部是**函数级、二分类、只测检测**；Martian 是 PR 级但仍只测检测；本仓是**PR/场景级、双轨、含契约语义**——第一次把「上游判空/分发表/IPC 所有权/启动期初始化」这类电信/嵌入式契约形态做成可机器判定的标准用例

## 1. 定位与原则

### 1.0 军规（第一原则）：真实 C/C++ 代码场景

> **本仓的一切用例都是真实、可编译的 C/C++ 代码——不是题目描述、不是伪代码、不是 markdown 里的代码片段。**

这条军规的操作含义：

1. 每个用例的 `src/` 是完整可编译的源文件集合（per-case CMake，统一 `-Wall` 无 error），golden 的锚点（anchor）必须指向真实语句；
2. 由此，消费方没有形态边界——**同一个用例可以同时被**：
   - Agent+LLM 评审（以 diff/PR 形态消费，我们的原生场景）；
   - 静态分析工具（以全量扫描形态消费：SA 团队把分析器直接指向 `src/`，**本地直跑**或**借 GitHub Actions 跑**，无需理解 PR 语义）；
   - 索引/提取类工具（以统一 compile_commands.json 消费：clangd / navmap / CSA / 任何需要编译数据库的工具）；
3. 任何「只能靠文字描述才能成立」的用例不得入库——场景的安全性或缺陷性必须在代码本体中可核验（契约则以 contract.yaml 数据随车）。

### 1.0.1 消费形态矩阵（消费方自助取用）

| 消费方 | 形态 | 入口 |
|---|---|---|
| SA / 分析器团队（本地直跑） | 全量扫描 | `cmake -S . -B build && <your-analyzer> build/compile_commands.json`，或用例级 `cases/<track>/<id>/src/` 直接指给分析器 |
| SA / 分析器团队（CI 跑） | GitHub Actions | 本仓提供可复用消费者 workflow（`consumers/github-action/`）：checkout 本仓 → 全量构建 → 调用你的工具 → 输出归一化 findings → 自动评分 |
| Agent+LLM 评审 | diff/PR | 用例 src 可物化为 `diff.patch`（空 base → 新增），或直接以源码树消费 |
| 索引/提取工具 | 编译数据库 | AllCases 一键 compdb；`context.navmap_expect` 提供提取结果的判定锚点 |

所有消费形态共用同一个输出契约：**归一化 findings（schema/findings.schema.json）→ tools/eval.py 评分**。消费方只需把自己的结果转成归一化格式。

## 1.1 双轨定义

### 1.1 双轨定义

| | Contract Track（契约抑制轨） | Defect Track（缺陷检出轨） |
|---|---|---|
| 回答的问题 | 不该报的别报 | 该报的全报 |
| 用例性质 | 「看着有缺陷、契约上安全」的负例 | 含真实缺陷的正例 |
| 核心指标 | FP 数、契约遵守率 | Recall、严重度分级正确率 |
| 防摆烂设计 | 混入 must_find 正例探「豁免过度」 | 混入 must_not_find 负例探「过度敏感」 |

### 1.2 原则（沿用 v0.1 并扩展）

| # | 原则 | 含义 |
|---|---|---|
| P1 | **薄而可复现** | 仓内只有用例、golden、契约、说明、最小评分器；一键编译、一键评分 |
| P2 | **工具无关** | 归一化 findings 接入面；golden 不含任何特定工具字段 |
| P3 | **可编译** | 每用例独立可编译；一键生成全仓 compile_commands.json |
| P4 | **契约显式** | 契约以 `contract.yaml` 数据化随车给出；「注入契约后仍报」单独记为契约违反 |
| P5 | **社区级质量** | Apache-2.0、贡献指南、用例评审流程、标注审计、基线报告、CITATION.cff |

### 1.3 明确不做

不做训练数据（SFT/DPO）、不做托管跑分服务、不做非 C/C++ 语言、golden 不收风格/可读性类主观判定。真实 CVE 修复 PR 轨（重资产，需双人盲标资源）列为二期，一期 Defect Track 用**合成 TP**（真实缺陷模式的最小复刻）先行。

## 2. 目录结构

```
cpp-review-bench/
├── README.md                  # 双轨说明、快速上手、引用方式
├── LICENSE                    # Apache-2.0
├── CITATION.cff
├── CONTRIBUTING.md            # 用例提交流程与评审标准（见 §7.3）
├── docs/
│   └── design-v0.2.md         # 本文件
├── cases/
│   ├── contract/              # Contract Track（16 例）
│   │   └── <case-id>/{src/,CMakeLists.txt,golden.json,contract.yaml,notes.md}
│   ├── defect/                # Defect Track（14 例）
│   │   └── <case-id>/{src/,CMakeLists.txt,golden.json,notes.md}
│   └── calibration/           # 校准子集（smoke test / FP 校准，不进主指标）
├── inbox/                    # harvest 候选入口（draft/confirmed/rejected 三态，见 §5.4）
├── schema/
│   ├── golden.schema.json
│   └── findings.schema.json
├── cmake/AllCases.cmake       # 一键全量构建 + 统一 compile_commands.json
├── consumers/
│   ├── github-action/         # 可复用消费者 workflow（工具团队 CI 里 `uses:` 即跑通 benchmark）
│   └── local/                 # 本地直跑入口（make 目标 / 脚本：构建 → 工具占位 → findings 转换样例）
├── tools/
│   └── eval.py                # 评分器（两层匹配，§4）
└── reports/                   # 各工具基线报告（持续积累）
    └── README.md              # 报告格式与登记流程
```

## 3. golden.json schema（v2，对齐 CQS golden comment 规范）

```json
{
  "id": "c21-startup-global-init",
  "track": "contract",
  "title": "全局变量启动期初始化",
  "version": 2,
  "languages": ["c"],
  "tags": ["nullguard", "lifecycle", "globalvar"],
  "expected": {
    "must_not_find": [
      {"scenario": "cwe-476", "note": "g_xxx 于启动期由 sys_init 分配，业务期按非空使用属契约安全"}
    ],
    "must_find": [
      {"scenario": "cwe-125", "severity": "important",
       "file": "src/uses.c", "anchor": "return g_cfg->mode;", "function": "current_mode",
       "line_tolerance": 3,
       "rationale": "len==0 时 g_cfg->mode 为越界读：分配 cap 为 0 的合法边界"}
    ]
  },
  "context": {
    "contract": "startup_init_contract",
    "callchain_hint": "uses g_nas_cfg; written once in sys_init",
    "navmap_expect": {"globalvar_writes": [{"var": "g_nas_cfg", "writer_pattern": "*init*"}]}
  },
  "difficulty": "medium",
  "split": "dev"
}
```

与 v0.1 的差异：

- **`must_find` 升级为 golden comment 结构**：`scenario + severity + file + anchor（语句级锚点文本）+ function + rationale`——与 research.md 的标注规范一致；`anchor` 优先于行号（C/C++ 行无语义，语句级锚点 + `line_tolerance`）
- **`severity` 必填**：评分时严重度分级错误单独统计（报对缺陷但定错级别 ≠ 完全正确）
- **`rationale` 必填**：为第二层 judge（轻量模型语义判等）提供基准文本；也强制用例作者写清「为什么这是缺陷」
- **`split: dev|test`**：防泄漏切分（test 子集默认不随仓公开细节说明，定期轮换）
- `track: contract|defect|calibration`

## 4. 评测协议（两层匹配，对齐 research.md §3.5）

```
L1 规则匹配（确定性）:
  scenario 精确/家族匹配（cwe-476 ≡ null pointer dereference 任意表述）
  file 精确匹配
  anchor 子串/规范化匹配（去空白）或 line ±tolerance
  function 精确匹配（可选加权）
L2 语义判等（可选，轻量 judge）:
  rationale 与工具输出的判定理由做语义判等
  judge 模型与版本写入报告元数据（防漂移）
附加维度:
  verified（工具用编译/最小复现验证过）→ 单列奖励
  severity 分级正确率 → 单列统计
  契约违反（注入 contract.yaml 后仍报 must_not_find）→ 单列，权重大于裸 FP
```

汇总报表：per-track pass 率、per-scenario/per-CWE 分层 P/R、FP 按「裸跑/契约违反」分布、FN 按缺陷类别分布。基线报告登记在 `reports/`（格式见 reports/README.md）。

## 5. 用例清单 v1（30 例）

### 5.1 Contract Track（16 例）

| # | id | 语言 | 场景（负例陷阱） | difficulty |
|---|---|---|---|---|
| 1 | `c01-upstream-nullguard` | c | 入口判空 → 4 层转发 → 解引用（单文件） | easy |
| 2 | `c01b-crossfile-nullguard` | c | 判空与解引用跨两文件 | medium |
| 3 | `c02-central-error-handling` | c | 错误码上层集中处理，本层不查返回值 | medium |
| 4 | `c03-assert-guard` | c | 自定义断言宏守护的解引用 | easy |
| 5 | `c21-startup-global-init` | c | 全局量启动期 malloc，业务期使用 | medium |
| 6 | `c06-flexible-array-member` | c | 变长结构体按 len 分配访问 | medium |
| 7 | `c08-protocol-offset-parse` | c | 协议解析步进偏移，入口统一校验 | medium |
| 8 | `c12-intended-wrap-seq` | c | 有意的序号回绕模运算（契约声明） | medium |
| 9 | `c15-spsc-lockfree-queue` | c | 单生产单消费无锁环形缓冲 | hard |
| 10 | `c16-enum-closed-switch` | c | 枚举全集闭合、故意无 default | easy |
| 11 | `t01-fnptr-table-1d` | c | 一维裸函数指针数组分发，入口判空 | hard |
| 12 | `t02-fnptr-table-2d` | c | 二维数组表，入口判空 | hard |
| 13 | `i01-ipc-ownership-by-pid` | c+mock | 按 PID 申请内存，释放归开辟方 | hard |
| 14 | `i06-ipc-handler-entrypoint` | c+mock | IPC 注册 handler 本地零调用（死代码陷阱） | medium |
| 15 | `m01-extern-c-entrypoint` | mixed | extern "C" 导出被 C 侧调用（死代码陷阱） | medium |
| 16 | `m03-c-guard-cpp-deref` | mixed | C 入口判空 → C++ 引擎解引用（跨语言） | hard |

### 5.2 Defect Track（14 例，全部 must_find）

| # | id | 真实缺陷 | scenario | difficulty |
|---|---|---|---|---|
| 1 | `r01-wrap-resume-bug` | 序号回绕时超时恢复选错点（rlc 式，跨函数逻辑） | cwe-190 | hard |
| 2 | `r02-offby-one-guard` | 长度守卫少算一字节（报文解析） | cwe-125 | easy |
| 3 | `r03-public-entry-bypass` | 判空仅覆盖单一路径，公开入口可绕过 | cwe-476 | medium |
| 4 | `r04-oob-write-stack` | 无界拷贝进定长栈缓冲 | cwe-787 | easy |
| 5 | `r05-early-return-leak` | early-return 路径未释放 | cwe-401 | easy |
| 6 | `r06-use-after-free-callback` | 回调注册后对象先释放，未反注册 | cwe-416 | medium |
| 7 | `r07-alloc-size-wrap` | `malloc(n*size)` 乘法回绕 → 小分配大写入 | cwe-190+787 | medium |
| 8 | `r08-missing-lock-increment` | 多线程计数器无锁递增 | cwe-362 | medium |
| 9 | `r09-double-free-errorpath` | 错误路径重复释放 | cwe-415 | medium |
| 10 | `r10-odd-length-bcd` | BCD 编码奇数长度越界读 | cwe-125 | easy |
| 11 | `r11-partial-stage-artifact` | 评审过的树 ≠ 提交的树（构建/集成类缺陷） | build | medium |
| 12 | `r12-signed-unsigned-compare` | 有符号/无符号混用致边界检查失效 | cwe-190 | medium |
| 13 | `r13-state-missing-transition` | 状态机合法事件无 handler（消息枚举有、表中没有） | logic | hard |
| 14 | `r14-buffer-len-source-untrusted` | 外部长度字段未约束即用于 memcpy | cwe-787 | medium |

> Defect Track 每例同时在 golden 中放 1 条 `must_not_find`（邻近但安全的代码点），探工具的「过度敏感」。

## 5.3 数据来源分级（Defect Track 的 TP 三级来源）

| 级 | 来源 | 标签正确率 | 成本 | 阶段 |
|---|---|---|---|---|
| **S（合成）** | 真实缺陷模式的最小复刻（§5.2 的 14 例） | 高（设计时已知） | 低 | v1 |
| **R（真实仓采集）** | **cpp-review-harvest**（自建自动入库仓，见其设计 v0.1）：CI 矩阵 schedule 驱动，多 SA 扫描开源仓 → SARIF 归一化 → ≥2 工具投票共识 → 用例草稿打包 → **bench inbox 三态人审**（confirm-tp 入 defect/ / confirm-fp 入 contract/（必填契约）/ reject 回流噪声画像） | 中（共识过滤 + 人审兜底） | 中 | v1.1 |
| **C（CVE 修复 PR）** | 三层金字塔 L1 管线：CVEfixes（NVD→commit 溯源）+ DiverseVul（issue 站锚点）+ SecVulEval（严过滤、语句级、2023-24 新 CVE），PrimeVul 质控工序（OneFunc/NVDCheck 初筛、归一化去重、时间序切分） | 高（双人盲标共识） | 高（标注资源） | v2（立项制） |

三级的 golden 共享同一 schema（§3），评分协议不变——**数据可追溯性要求**：R/C 级用例在 notes.md 中记录来源 commit、命中的工具、人审人。

**公开数据集分工参照**（沿 research.md §3.6 结论，papers/ 已归档原文）：CVEfixes = L1 主源（结构化最好）；DiverseVul = CWE 分布参照（自报标签仅 60% 可信）；SecVulEval = 过滤管线模板 + 语句级锚点 + 新 CVE 源；PrimeVul = 质控方法论（不提供数据）；CASTLE = calibration/ 校准子集形态（`// {!LINE}` 标注格式可借鉴）；CQS/Meta = 标注与评估协议骨架（修正其三个偏差：ground truth 不由被测模型生成、标注者盲标、补全 recall 分母）；Devign/Big-Vul = 反面教材（粗粒度标签 24-25% 正确率）。

## 4.1 与 Martian Code Review Bench 的对齐

- **报告兼容**：v1.1 起 eval.py 支持输出 Martian 形态报表（per-PR findings + precision/recall），Defect Track 的 R/C 级用例可与 Martian 的 50 PR 交叉对照
- **从 OCR 第三方复测学到的四条**（`open-code-review/docs/src/appendix/benchmark.md`）：① 评测版本必须钉死并在报告中注明；② 子集/全量分开报告，不混用；③ ground-truth 判定口径必须显式文档化（本仓 §3.3 判定语义 + 口径审计记录公开）；④ recall 粒度（逐条 finding vs 逐 PR）在报表中显式标注

## 5.4 自动入库管线（cpp-review-harvest）

benchmark 的 R 级数据由独立仓 `cpp-review-harvest` 自动采集（设计见其 docs/design-v0.1.md）：CI 矩阵（repo × tool）定时扫描开源仓 → 共识投票 → 用例草稿 → 本仓 `inbox/`。人审三态：confirm-tp → `cases/defect/`；confirm-fp → `cases/contract/`（**必填 contract.yaml**——每条 FP 确认即一条真实世界 exemption_pattern 入库）；reject → 回流采集器噪声画像。双轨由同一管线喂养是其核心设计。

## 6. 构建与编译保证

- 每用例独立 `cmake -S cases/<track>/<id> -B build/<id>`；`cmake/AllCases.cmake` 一键全量并输出统一 `compile_commands.json`
- IPC/混合用例的 `src/mock/` 提供契约 API mock 头（`ipc_alloc/ipc_send/ipc_register_handler`、extern "C" 桥接宏），列入 compdb
- C 用例 `-std=c11`、C++ 用例 `-std=c++17`，统一 `-Wall`；r08 并发用例链接 `-pthread`

### 6.1 可复用消费者 workflow（consumers/github-action/）

为工具团队提供的「三行接入」：

```yaml
jobs:
  bench:
    uses: <org>/cpp-review-bench/consumers/github-action/bench.yml@v1
    with:
      tool_command: 'my-analyzer --sarif-out=out.sarif $COMPDB'   # 工具方只填这一行
    secrets: inherit
```

workflow 内部：checkout 本仓（pinned ref）→ AllCases 构建 + compdb → 执行 `tool_command` → 归一化 findings → eval.py 评分 → 上传报告 artifact 并（可选）登记到 reports/ 候选。工具团队不需要理解用例格式、双轨结构或评分细节——**填一行命令，拿回一份报告**。

## 7. 社区化治理

### 7.1 质量闸门（建成线）

- [ ] 30 例全部编译通过 + 统一 compdb；golden 全过 schema 校验
- [ ] 每例 notes.md 三段式（真实仓形态 / 为什么契约安全或真缺陷 / 各工具误判方式）
- [ ] 标注审计：每类抽 ≥3 例第三方复核 golden 判定，出审计记录（对齐 research.md「每 CWE 类抽 50 复核」的等比缩小版）
- [ ] ≥2 个真实工具全量基线报告入 `reports/`（建议：agent-reviewer 场景评审 × 1 个 SA/CSA 类工具）
- [ ] eval.py 对构造 findings（故意含 1 FP + 1 FN + 1 契约违反）输出正确判定

### 7.2 防泄漏

- `split: dev|test`；test 子集 golden 细节不展开于公开文档，定期轮换（变更标识符/等价改写）
- 用例代码均可等价改写生成变体（二期：变体生成器）

### 7.3 贡献与评审流程（CONTRIBUTING.md 要点）

- 新用例五文件齐备 + 自评表（场景出处、契约可验证性、预期误判面）
- 双人复核制：任何 golden 变更需第二名复核者签字（借鉴 research.md 双人共识协议）
- 用例状态机：`draft → reviewed → active → retired`（与契约库的 quarantine 治理同构）

### 7.4 公信力约定（工程治理，非对外动作）

基准的公信力来自可复现与口径公开，与任何对外曝光动作无关：

1. **可复现**：`README` 一条命令完成「构建 → 评分 → 出报表」；评测环境（编译器版本、judge 模型版本）钉死并记录
2. **口径公开**：判定语义（§3.3）、golden 评审记录、标注审计报告全部随仓公开——「标注主观性」是 review benchmark 的第一质疑点，用公开对冲
3. **欢迎复测**：reports/ 接受第三方复测 PR；基准数字一律标注「自测口径」，不做绝对质量承诺

## 8. 演进路线

| 阶段 | 内容 |
|---|---|
| **v1（当前）** | 30 例双轨 + 评分器 + ≥2 工具基线 |
| v1.1 | R 级数据接入（cpp-review-harvest 采集管线）；Martian 兼容报表；consumers/github-action 上线（≥1 个外部 SA 工具经三行接入跑出报告）；用例变体生成器；navmap_expect 全量化 |
| v2 | Defect Track 接入真实 CVE 修复 PR（research.md 三层金字塔 L1/L2，需双人盲标资源立项） |
| v3 | online 轮换轨 + 多配置变体（同一用例多 -D 编译形态） |
| 远期 | 社区排行榜（reports/ 聚合多工具基线） |

## 9. 命名与品牌

- **仓名**：`cpp-review-bench`（描述性、可搜索）；备选 `cr-bench` / `ccrb`
- 一句话定位（README 首行）：*A dual-track benchmark for C/C++ code review: measure both what your reviewer catches and what it wrongly flags.*
- 差异化标签：`dual-track` `contract-aware` `mixed C/C++` `PR-level` `telecom-grade`
