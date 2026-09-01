# cpp-review-bench 代码仓独立分析（思路 / 逻辑 / 改进点）

> 本文是一份**独立审读文档**：对仓的整体思路与实现逻辑做通盘解释，并列出可改进点。
> 它不修改任何既有设计文档与代码，也不构成权威设计事实源；与 `docs/design-v0.4.md`、
> `harvest/docs/design-v0.1.md` 冲突时，以对应设计文档为准。
> 生成方式：通读全部源码（schema / eval / sa adapters+runners / harvest / workflows / reports / cases 抽样）。
> 所有引用以 `file:line` 标注，均为审读当日的代码现状。

---

## 1. 一句话定位

**cpp-review-bench 是一个「C/C++ 代码评审工具」的评测基准**：把「评审工具该抓到什么 / 不该报什么」
做成一套**真实、可编译**的用例 + 一个**确定性评分器** + 一套**工具接入层**，并配一条**自动采集真实
缺陷候选的流水线（harvest）**来持续扩仓。

它与所有被测对象（静态分析工具 / LLM 评审 / 上下文提取工具）**解耦**：任何工具只要把自己的输出
归一到本仓定义的 `findings` 格式，就能被 `tools/eval.py` 打出四态分数。

---

## 2. 核心思想

### 2.1 双轨：同时测「漏报」和「误报」

传统漏洞检测基准（PrimeVul / DiverseVul / CASTLE）基本都是**函数级、二分类、只测检测（recall）**。
本仓的差异化在于**双轨**：

| 轨 | 回答的问题 | 用例性质 | 核心指标 |
|---|---|---|---|
| **Contract Track 契约抑制轨** | 不该报的别报（FP 抑制） | 「看着有缺陷、但契约上安全」的负例（如上游判空、分发表、IPC 所有权、启动期初始化） | FP 数、契约遵守率 |
| **Defect Track 缺陷检出轨** | 该报的全报（TP 检出） | 含真实缺陷的正例 | Recall、severity 分级正确率 |

两条轨各自埋了「防摆烂」探针：
- Contract 轨混入 `must_find` 正例 → 探「用一刀切豁免把所有东西都放过」；
- Defect 轨混入 `must_not_find` 负例 → 探「过度敏感、邻近安全点也报」。

这是本仓最核心的设计判断：**只测 recall 的基准会让一个「什么都报」的傻工具刷高分**，所以必须
把 FP 和契约语义（contract.yaml）纳入可机器判定的指标。

### 2.2 军规：一切用例都是真实可编译代码

不能用题目描述 / 伪代码 / markdown 片段，因为：

1. 让同一个用例能被**多种消费形态**复用（见下）；
2. 场景的安全/缺陷性必须在**代码本体 + contract.yaml 数据**中可核验，不靠文字兜底。

### 2.3 三种消费形态，一个输出契约

| 消费方 | 形态 | 入口 |
|---|---|---|
| SA 团队（本地直跑） | 全量扫描 | `cmake -S . -B build` → `build/compile_commands.json` |
| SA 团队（CI 跑） | GitHub Actions | `.github/workflows/ci.yml` |
| Agent+LLM 评审 | diff/PR | 用例物化为 `diff.patch` 或直接读源码树 |
| 索引/提取工具 | 编译数据库 | `AllCases.cmake` 一键 compdb + `context.navmap_expect` |

所有形态**共用同一个归一化输出**：`findings.json`（schema/findings.schema.json）→ `tools/eval.py`。
这就是「薄而可复现、工具无关」原则（design §1.2 P1/P2）的落地。

---

## 3. 体系架构与数据流

### 3.1 目录职责

```
cpp-review-bench/
├── cases/           # 双轨用例（contract 16 + defect 14），每例五文件
├── schema/          # golden.schema.json(v2) + findings.schema.json（冻结点）
├── tools/           # eval.py(评分器) + check_cases.py(自检)
├── cmake/           # AllCases.cmake 一键全量 + 统一 compdb
├── sa/              # 9 工具接入层：adapters / runners / scripts / harnesses / docker
├── harvest/         # 采集管线：真实缺陷候选生产线 + 人审移植流水线
├── reports/         # 各工具基线报告（含每轮证据 raw json）
└── .github/workflows/  # bench CI（ci.yml）+ harvest 管线（harvest*.yml）
```

### 3.2 主数据流（评分闭环）

```
任意工具输出 ──(adapter)──> 归一化 findings.json ──(eval.py)──> 四态 P/FN/FP/EXTRA
                                  ▲                                   │
                                  │归一化 schema 是唯一契约           ▼
                          golden.json（真值）              per-track pass/recall/severity 报表
```

### 3.3 采集数据流（harvest 飞轮）

```
每日 cron(harvest.yml)
  └─ matrix(source × repo) 爬 GitHub 已合并 fix-PR（pr-mining）
      └─ 切「修复前」函数闭包 → judge 启发式推 scenario → 配额限流
          └─ vote（当前单源退化 min-tools=1）→ pack_case 打包五文件草稿
              └─ harvest/inbox/draft/（= 线索 + 移植 blueprint，非半成品用例）
                  └─ 人审三态：confirm-tp→cases/defect | contract→cases/contract | reject→噪声画像
                      └─ 入 cases/ 后再由 ci.yml 9 工具实测能否检出（飞轮闭环）
```

---

## 4. 各模块详解

### 4.1 `cases/`：双轨用例与五文件

每例五文件齐备：
- `src/`：真实可编译源码（自然风格，无「试验/播种」字眼）
- `CMakeLists.txt`：per-case 独立构建（C 用 `-std=c11`、C++ 用 `-std=c++17`）
- `golden.json`：真值标注（v2 schema）
- `contract.yaml`：仅 contract 轨，数据化契约（name/guarantees/assumptions/exemptions）
- `notes.md`：三段式（真实仓形态 / 为什么契约安全或真缺陷 / 各工具误判方式）

**用例覆盖两轴**：单文件→跨文件→跨语言（C/C++ 混编），以及 IP 分发表/无锁队列/状态机等
电信/嵌入式契约形态。contract 轨每例在「看着像缺陷」处放 `must_not_find`（如 `c01` 的
`return ie->len;`，安全理由：入口判空 + 唯一静态调用链），同时在别处放一个真实 `must_find`
（如 `c01` 的 `uint8_t total += ...` 整数回绕）。

**契约的机器可判定性**（核心创新点）：`contract.yaml` 把「为什么这里是豁免」写成结构化数据
（guarantees/assumptions/exemptions 锚定到具体 file+anchor），使「注入契约仍报」能被程序化判定为
「契约违反」（权重高于裸 FP），而不是靠人工读 notes 判断。

### 4.2 `schema/`：两个 schema 构成唯一契约

- `golden.schema.json`（v2，`schema/golden.schema.json:1`）：golden 标注。`mustFind` 必填
  `scenario + severity + file + anchor + function + rationale`；`anchor` 优先于行号（`line_tolerance`
  兜底）；`scenario` 取自由字符串（模式 `cwe-N|build|logic` 且允许 `+` 组合，如 `cwe-190+cwe-787`）；
  `severity` 三级枚举；`split: dev|test` 防泄漏。
- `findings.schema.json`（v1，`schema/findings.schema.json:1`）：归一化 findings。一条 finding 必填
  `file + anchor`，可选 `scenario/severity/function/line/message/verified`。`scenario` 可空
  （工具不知道 CWE 时置 null，eval 退化为 file+anchor 存在性匹配）。

两 schema 都 `additionalProperties: false`，是仓内「契约冻结点」——任何变更必须先回主会话讨论。

### 4.3 `tools/eval.py`：两层匹配 + 四态 + 汇总

匹配协议（design §4 的两层）：

- **L1 确定性匹配**（`eval.py:70` `finding_hits_must`）：
  1. scenario 家族匹配（`scenario_family_match`，`eval.py:40`）
  2. file 精确
  3. anchor 去空白互为子串（优先）或 line ∈ [gline±tolerance]（兜底）
  4. function 精确（golden 有 function 且 finding 也有 function 时才强校验）
- **L2 语义判等**：rationale vs finding.message 的轻量 judge——**接口预留，默认关**（未实现）。

四态（`eval.py:169`，per case）：`PASS / FN / FP / EXTRA`。
- `must_find` 漏报 → FN；`must_not_find` 被报 → FP；未被吸收的 finding → EXTRA。
- FP 再细分：contract 轨 + `contract_injected` → `contract_violation`（权重高）；否则 `bare_fp`。

附加维度：severity 分级正确率、`verified` 计数。

**自检**（`eval.py:301` `synthetic_findings`）：构造「1 契约违反 + 1 EXTRA + 1 FN + 1 line 兜底命中」
四组 findings，`eval.py selftest` 验证判定正确——这是 design §7.1 建成线之一。

### 4.4 `tools/check_cases.py`：入库自检

遍历所有 `cases/*/*/golden.json`（`check_cases.py:15`），校验：golden 过 schema / id 与目录名一致 /
track 与所在轨目录一致 / 每个 anchor+file 真实存在（去空白子串） / contract 轨带 contract.yaml 且
契约名与 `context.contract` 一致。任何一条不符即非零退出挂 CI。

### 4.5 `cmake/AllCases.cmake`：一键全量 + 统一 compdb

顶层 `CMakeLists.txt` include `cmake/AllCases.cmake`。后者对每个 `cases/*/*/src/*.c` 建一个独立
`STATIC` 静态库目标并统一 `-Wall`，从而生成**单份 `compile_commands.json`** 供 clangd/navmap/CSA 消费。

### 4.6 `sa/`：9 工具归一层

| 模块 | 内容 |
|---|---|
| `adapters/` | 9 个 `*_to_findings.py`：把各工具输出转归一化 findings；`sarif_to_findings.py` 是通用 SARIF→findings；`findings_to_sarif.py` 是反向展示层（PR 内联标注） |
| `runners/` | 7 个 `run_*.sh`：CSA(singletu/ctu)/CppCheck/clang-tidy/Infer/KLEE/Joern 的触发脚本，本地与 CI 共用 |
| `scripts/joern/scan.sc` | Joern CPG 通用查询（危险调用枚举，不读 golden） |
| `harnesses/` | 11 个 KLEE 符号执行入口（按 case 归置 `klee_harness.c`） |
| `docker/cooddy/` | Cooddy 镜像构建（z3 静态链接修复史） |

**adapter 的通用模式**（以 `csa_to_findings.py` 为例）：解析工具原始产物 → 把绝对路径归一为相对
case 根的 `src/...` → 用「源文件对应行内容 strip」作为 anchor（因为 CSA 无 CWE 标签，scenario 置 null，
让 eval 退化为 file+anchor 匹配）→ 产出 schema 合规的 findings doc。

**已修复但待重跑的点**：`klee_to_findings.py` / `joern_to_findings.py` 早期存在「读 golden 反写 findings」
的自证问题，当前代码已改为「只从 KLEE 真实 error / Joern 通用查询产出」（见 `klee_to_findings.py:8`
头注、`scan.sc:2`），但 `reports/analysis-report.md:31` 仍标注其 recall 数字**待 adapter 修复后重跑**。

### 4.7 `harvest/`：采集管线 + 人审移植

见 §3.3 数据流。要点：
- **draft ≠ 用例**：draft 是「线索 + 移植 blueprint」。`src/` 是原始切片（不可直接编译），
  `notes.md` 六段（溯源表 / 缺陷描述与触发条件 / 真实修复 diff / 移植要点 / contract 契约安全 /
  accept 检查清单六项）。
- **license/port 策略**：候选顶层带 `license`+`port`（direct=宽松许可直接移植；rewrite=只许参考重写），
  只进 notes 不进 golden（golden context 冻结）。
- **场景配额**：`max_per_scenario`（默认 5）防单一仓单一缺陷类型刷屏。
- **fp-mining**：可选第二轮抓「修静态分析误报」PR，产出 contract 轨 must_not_find 候选。
- **策略 1/2**：同文件闭包切片 + 可编译性打分（dep_count / gcc `-fsyntax-only` 错误数），
  使 inbox 列举顺序即移植优先级。
- **人审三态**：`/case accept|contract|reject` 评论指令 → harvest-review.yml 机械 git mv，
  **不校验移植是否完成**（因此必须先移植重写 → 过 check_cases + 全量编译 → 再流转）。

### 4.8 workflows 与 reports

- `ci.yml`：bench 主 CI。`build-and-eval` job（全量构建 + check_cases + eval selftest + CSA/CppCheck/
  clang-tidy）+ infer/codeql/klee/codechecker/joern/cooddy-verify 各独立 job。
- `harvest*.yml`：采集/打包/复核/SARIF 辅助 + cooddy 镜像构建。
- `reports/`：基线报告（baseline-v1/v2、analysis-report）+ 每轮证据 raw json。报告必含
  「工具版本钉死 / 配置 / 环境 / 判定口径 / 汇总表 / 复现命令」（`reports/README.md`）。

---

## 5. 评测协议的两个关键细节（值得理解）

### 5.1 anchor 优先于行号

C/C++ 行号无语义（多行语句、宏展开后行漂移），所以 golden 用**语句级 anchor 文本** + 去空白匹配为
主，`line_tolerance` 只在 anchor 无法命中时兜底。这是对齐 research.md「golden comment」标注规范的取舍。

### 5.2 severity 单列、契约违反加权

「报对了缺陷但定错级别」不算完全正确（severity 正确率单列）；「注入契约后仍报 must_not_find」记为
契约违反、权重 > 裸 FP。这两点把「只测召回」的单一维度拆成了可独立归因的多个指标。

---

## 6. 设计与工程优点（值得保持）

1. **双轨 + 防摆烂探针**是扎实的设计，避免「只测 recall 被刷分」这一常见基准缺陷。
2. **工具无关的薄契约**：findings schema + eval 是唯一的接入面，9 个异构工具都走同一条路。
3. **anchor 优先**比纯行号匹配对 C/C++ 更鲁棒。
4. **自检闭环**：`check_cases.py`（golden 锚点真实性）+ `eval.py selftest`（四态判定），
   都在 CI 挂门，成本低、防退化。
5. **harvest 的一条管线两头收货**：真缺陷→defect、FP 候选→contract（带真实工具误报证据），
   是 Contract Track「真实世界契约」的稀缺来源。
6. **可复现承诺**：版本钉死 + pinned ref + 判定口径公开 + 第三方复测 PR 机制。
7. **可移植性工程细节**：脚本大量处理了 CI `LANG=C` 下中文 stderr 崩溃、GitHub search 限流/分片/
   预算闸等真实坑，实践血统充分。

---

## 7. 可改进点 / 问题清单

> 按「严重度 → 类别」组织。标注面对的均为审读当日代码，未改仓。

### 7.1 敏感信息 / 合规（军规违反，应优先修）

1. **`sa/adapters/findings_to_sarif.py:150` 硬编码真实 GitHub 用户名**：
   `"informationUri": "https://github.com/Atituiset/cpp-review-bench"`。
   违反 CLAUDE.md「无敏感信息：不得含 GitHub 用户名/仓库地址（对外场景用 `<org>` 占位）」。
   对比 `harvest/config/repos.yaml:91` 的 aetherstack 正确用了 `<org>/aetherstack` 占位。
   该 URI 应改为 `<org>/cpp-review-bench` 或空串。

2. **机器缓存产物被提交进 git**：`.cache/clangd/index/*.idx`（clangd 索引缓存）处于追踪状态
   （`git ls-files .cache/` 有输出），但 `.gitignore`（`.gitignore:1`）只忽略了 `build/ __pycache__/`，
   未忽略 `.cache/`。应加 `.cache/` 并从版本库移除。

### 7.2 实现与设计/注释不一致

3. **`AllCases.cmake:24` 只 `file(GLOB *.c)`，漏掉 C++ 源**：混合编程用例 `m01`（entry.cpp）、
   `m03`（bind.cpp/engine.cpp）的 `.cpp` 文件**不会进入 `compile_commands.json`**。
   实测 `build/compile_commands.json` 仅 4 条全 `.c`（且是早期「前 3 例」的陈旧产物）。
   后果：设计 §1.0.1 承诺的「索引/提取工具以统一 compdb 消费」对 C/C++ 混编用例的 C++ 侧不成立；
   CSA 等按 compdb 扫描时也扫不到 `.cpp`。应改为 `file(GLOB .../*.{c,cc,cpp,cxx})`。

4. **`eval.py::scenario_family_match`（`eval.py:40`）注释说「家族等价（cwe-476 ≡ 任意表述）」，
   实现却是 `golden_s == finding_s` 的精确相等**。既没有「家族归一」，也没有对 `cwe-190+cwe-787`
   组合场景的展开匹配。后果：r07 的 `must_find scenario="cwe-190+cwe-787"` 与工具报的单个
   `cwe-190` 或 `cwe-787` **永远不匹配**（反之，若工具报 `cwe-190` 还会命中 r07 的
   `must_not_find "cwe-190"`）。建议：要么实现真正的家族/组合匹配，要么在 design 里明确
   scenario 只支持精确相等并把组合场景拆成两条独立 must_find。

5. **`AllCases.cmake:9` 注释写「不做链接（EXCLUDE_FROM_ALL）」，但 `add_library` 未加
   `EXCLUDE_FROM_ALL`**。当前无实际危害（CI 恰需要 build 全量），但注释与实现矛盾，易误导。

6. **`vote.py:76` 注释说「line 在 ±tol 内计同位置」，实现只按 `(file, scenario)` 聚类**
   （`vote.py:78-82`），完全没有行号距离判断。两工具对**同一场景但不同行**报出，会被误判为
   「≥2 工具共识」。当前因单源退化（min-tools=1）未暴露，sa-scan 上线恢复 ≥2 共识后这是个实打实的错误。

### 7.3 契约违反 / 裸 FP 的判定语义未真正落地

7. **`eval.py:233` `contract_injected = bool(golden.context.contract)` 恒为 True**：
   所有 contract 轨 golden 都带 `context.contract`，因此 `cmd_eval`/`cmd_run` 走真实跑分路径时，
   contract 轨的任何 FP **永远被记为 `contract_violation`，永远无法出现 `bare_fp`**。
   只有在 `selftest`（`eval.py:364`）手动传 `contract_injected=False` 才能演示区别。
   但 design §4 说「注入契约后仍报」是与「未注入契约的裸跑」相对照的——当前 eval 协议/CLI
   **没有暴露「同一次工具产出分别按注入/未注入算两次」的路径**，dependency 维度名存实亡。
   建议：eval 加 `--no-contract` 开关（或对同一 findings 同时输出两种口径），否则「裸 FP vs
   契约违反」的汇总列（`eval.py:263`）在真实跑分里恒为 0/全号，报表有误导性。

### 7.4 健壮性 / 边界语义

8. **`must_not_find` 无 anchor 时的语义过宽**（`eval.py:127-130`、`eval.py:160`）：
   schema 允许 `mustNotFind` 不填 file/anchor。一旦不填 anchor，eval 把「该 file 内任意 finding」
   都记为 FP 并吸收进 EXTRA 排除集。defect 轨若某 must_not_find 漏了 file+anchor，会把整文件发现
   全判 FP。建议：schema 或 eval 显式约束无 anchor 的 must_not_find 的适用范围，避免静默扩大惩罚。

9. **EXTRA 的吸收用 `(file, local_index)` 键**（`eval.py:146`）本身正确，但 `by_file` 的索引键是
   finding 的 `file` 字段，而 finding 的 `file` 与 golden 的 `file` 口径一致性依赖 adapter 归一。
   若某 adapter 忘记归一（如写了绝对路径），L1 的 file 精确匹配会静默全 FN。建议：eval 对
   findings 的 file 做一次「归一为相对 case 根」的防御性处理或在 check 阶段报警。

10. **`locate_gline`（`eval.py:51`）取 anchor「首个匹配行」**：多行同文本/宏展开场景可能定位错行，
    使 line 容差兜底落在别处。属低概率，但可记录。

### 7.5 覆盖 / 完成度（多为规划项，已在文档中披露）

11. `consumers/` 消费入口未建（design §6.1）——工具团队「三行接入」仍是蓝图，现由 ci.yml +
    `sa/runners` 承担。
12. `cases/calibration/` 规划未建（design §2）。
13. 标注审计（每 CWE 类第三方复核）未做（design §7.1）。
14. `harvest` 的 sa-scan 采集源为占位（`normalize.py:30` `normalize_sarif` 返回空、`build_compdb.sh`
    等脚本未实现），vote 因此单源退化 min-tools=1。
15. `rules.yaml` 的 `scenario_map / noise_blacklist / single_tool_high_conf / unmapped_bucket`
    均标注「规划项未接线」。
16. KLEE/Joern adapter 已修复但 reports 数字待重跑；`reports/evidence/klee/` 只落盘 2 个 json，
    与「11/11」声明不符（已在 analysis-report 自曝）。
17. Martian 兼容报表（v1.1 规划）未实现。

### 7.6 测试覆盖

18. **除 `eval.py selftest` 与 `check_cases.py` 外，几乎无自动化单元测试**：harvest 的
    `pr_mine/vote/pack_case/normalize` 全靠 CI 实跑验证，闭包切片 / judge 启发式 / dep_count 等
    纯函数逻辑没有 pytest 级回归。建议至少对 `slice_before/closure_slice/judge_bug/extern_dep_count`
    和 `eval_case` 的各边界（组合 scenario、无 anchor must_not_find、severity 缺失）补单测。

19. **适配器缺 golden 级测试**：9 个 adapter 是否产出合规 findings、是否与 golden 对齐，没有
    固定 fixture 测试，回归只能靠 CI 重跑。

### 7.7 细节 / 一致性

20. `check_cases.py:41` 用 `norm(anchor) not in norm(整个文件内容)` 判断 anchor 存在，
    理论上可被跨行拼接误导（anchor 去空白后恰好等于两行内容拼接）。极低概率，可改为逐行判断
    （与 `eval.py::locate_gline` 同口径）。

21. `findings_to_sarif.py:150` 的 URI 问题（同 §7.1-1）之外，该文件 `LEVEL` 映射把 `critical/important`
    都压缩成 SARIF `error`（`findings_to_sarif.py:27`），severity 三级信息在 SARIF 展示层坍缩，
    与 eval「severity 单列」的精度诉求不完全对齐（属展示层降级，可接受但应记录）。

---

## 8. 优先级建议

| 优先级 | 事项 | 对齐小节 |
|---|---|---|
| P0 | 移除真实用户名 + 提交物清理 `.cache/` | §7.1 |
| P0 | `AllCases.cmake` 补 `.cpp` 到 compdb（混合用例 C++ 侧断裂） | §7.2-3 |
| P1 | eval 补「未注入契约」对照口径，让「裸 FP vs 契约违反」真正有区分度 | §7.3-7 |
| P1 | scenario 家族/组合匹配：实现与文档对齐（否则 r07 组合场景恒 FN） | §7.2-4 |
| P1 | vote 行号聚类修正（sa-scan 上线前埋单测） | §7.2-6 |
| P2 | harvest 纯函数单测 + adapter golden 测试 | §7.6 |
| P2 | must_not_find 无 anchor 的语义边界显式化 | §7.4-8 |
| P3 | 其余规划项（consumers/calibration/标注审计/sa-scan/Martian 报表） | §7.5 |

---

## 9. 总结

本仓是一个**结构清晰、契约严谨**的 C/C++ 评审基准：双轨设计 + 真实可编译用例 + 薄归一化契约 +
确定性评分器 + 自动化采集飞轮，已经完整跑通 30 例 × 9 工具的基线，并从中提炼出「哪些缺陷类型
当前 SA 集体失明」这一有价值的结论（`reports/analysis-report.md`）。

主要改进方向集中在三层：**(1) 合规卫生**（真实用户名、缓存产物入 git）；**(2) 实现与设计对齐**
（compdb 漏 C++、scenario 组合匹配、契约违反对照口径、vote 行号聚类）；**(3) 测试覆盖**
（harvest 与 adapter 缺单测）。这些都不动设计冻结点，属于工程收敛层面，修起来风险可控。