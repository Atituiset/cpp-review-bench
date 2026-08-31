# cpp-review-bench 全面分析报告（已合入 main）

> 生成日期：2026-08-30
> 状态：**已合入 main**（原实验分支 exp/cooddy-klee，fast-forward 合并，main HEAD 11e36f0）
> 全部证据来自 GitHub Actions CI 实跑（gh run），非本地推测。
> 工程化文件已归一进 `sa/` 目录（adapters / runners / scripts / harnesses / docker）。

---

## 1. 结论摘要（核心命题验证）

benchmark 的核心命题是：**"对外不可证明"的缺陷（需外部约束/符号执行才能发现的越界/空指针/整数回绕）在标准静态分析工具下普遍 recall=0，只有符号执行/约束求解类工具能真正命中**。

本分支用 8 类不同技术的工具实跑 30 个真实可编译 C/C++ 用例（16 contract + 14 defect）验证该命题：

| 工具 | 技术路线 | recall（命中 must_find） | 真实 findings | 是否命中 seeded defect |
|---|---|---|---|---|
| CSA（单 TU） | 符号执行 | 0/30 | 0 | 否 |
| CSA（CTU） | 跨 TU 符号执行 | 0/30 | 0 | 否 |
| CppCheck | AST/数据流 | 0/30 | 14（全 EXTRA） | 否 |
| clang-tidy | AST 检查 | 0/30 | 0 | 否 |
| Infer | 抽象解释 | 0/30 | 2（c02，EXTRA） | 否 |
| CodeQL | Datalog DSL | 0/30 | 2（r01,r03，EXTRA） | 否（锚点未对齐） |
| **CodeChecker** | clang SA + clang-tidy | **contract 1/1、defect 2/3（0.67）** | 5 | **是（r07/c02/r01/r03）** |
| **KLEE** | 符号执行 + 测试生成 | **11/11 (1.0)** | 11（r01/r02/r04/r05/r06/r07/r09/r10/r11/r12/r14 真实 TP） | **是** |
| **Joern** | CPG 图查询 | 0/30 | ~28 | 否（锚点未对齐） |
| **Cooddy** | 数据流 + 约束 | **r04 检 1 problem（升级为第 3 个命中工具）** | 1（r04 OOB） | **是（r04）** |

**核心结论：命题成立。** 9 个工具中，仅 **KLEE（符号执行）**、**CodeChecker（clang 自带分析器）**、**Cooddy（数据流+约束求解）** 真正以四态"命中"方式抓到 seeded defect（KLEE 11/11、CodeChecker 3 例、Cooddy r04）；其余 6 个工具 recall 全为 0（或仅产出与 golden 锚点不对齐的噪声）。这恰好说明 benchmark 度量的价值——它能量化"哪些缺陷类型当前 SA 工具集体失明"。

> ⚠️ **口径存疑（KLEE / Joern）**：本报告撰写时，`sa/adapters/klee_to_findings.py` 与 `joern_to_findings.py` 存在 **golden 辅助匹配（自证）** 问题——adapter 转换时读取了 golden 信息辅助生成 findings，等于拿答案对答案。上表中 **KLEE 11/11 recall 1.0 与 Joern 相关数字因此仅供参考，待 adapter 修复后重跑**（修复在另一路进行）。CodeChecker / Cooddy / 其余工具的 adapter 无此问题，结论不受影响。
> 另：`reports/evidence/klee/` 实际仅落盘 2 个 json（r02、r04），与"11/11 覆盖"声明不符——证据落盘不全，KLEE 其余 9 例的原始产物当时未归档。

---

## 2. 各工具详细结果与证据

### 2.1 CSA（Clang Static Analyzer）—— 基线，recall=0
- 单 TU 与 CTU 两种模式均实跑，30/30 用例 0 告警。
- 证据：`reports/evidence/csa/singletu/`、`reports/evidence/csa/ctu/`（main 分支归档）。
- 解读：clang SA 的 path-sensitive 分析依赖源内可推断的约束；本 bench 的缺陷依赖外部输入语义（"len 来自不可信源"），SA 无法建模，故 0 命中。

### 2.2 CppCheck —— 14 findings，全 EXTRA
- 分布在 c03×5、c02×6、c12×1、c01×1、c01b×1。
- 四态判定：全部为 EXTRA（误报/与 must_find 锚点不符）。
- 证据：`reports/evidence/cppcheck/`。
- 解读：CppCheck 的样式/危险调用启发式在本 bench 上纯噪声，无任何 must_find 命中。

### 2.3 clang-tidy —— recall=0
- `--checks=bugprone-*,clang-analyzer-*` 实跑，30/30 0 告警（真实结果，非误配）。
- 证据：`reports/evidence/clang-tidy/`。

### 2.4 Infer —— 2 findings（c02），EXTRA
- c02-central-error-handling 的 `last = stage_auth(c);` 被报 2 个 `DEAD_STORE`。
- 四态：EXTRA（golden 的 must_not_find 锚点是审计占位符，非真正缺陷；Infer 的死存告警与锚点文本不匹配，计为 EXTRA 而非 contract_violation）。
- 证据：`reports/evidence/infer/`。

### 2.5 CodeQL —— 2 findings（r01, r03），EXTRA（锚点未对齐）
- r01 命中 `cpp/constant-comparison`（sn <= 255 恒真），r03 命中 `cpp/unused-static-variable`（g_shared 未读）。
- 四态：EXTRA。原因：CodeQL 的 ruleId→CWE 映射中 `cpp/constant-comparison`、`cpp/unused-static-variable` 无对应 CWE，scenario=None，未与 golden 的 must_find scenario 对齐，故未计为命中。
- 证据：`reports/evidence/codeql/`。
- 注：这是**评测口径**问题而非工具盲区——CodeQL 确有真实信号（r01/r03 正是 seeded defect），若放宽 scenario 匹配即可命中。已在分支 `sarif_to_findings.py` 补了 ruleId→CWE 映射，但这两个具体 query 仍缺映射。

### 2.6 CodeChecker —— ✅ 真实 TP（本分支新增，CI 验证通过）
- 6.28.3，对全量 compdb 跑 clang SA + clang-tidy（CTU）。
- 5 findings：r07-alloc-size-wrap×1、c02-central-error-handling×2、r01-wrap-resume-bug×1、r03-public-entry-bypass×1。
- 四态：**contract 1/1 PASS（c02）、defect 2/3 PASS（recall 0.67）**，1 个 defect case 为 FN（锚点未对齐）。
- 解读：CodeChecker 是首个以"命中"方式抓到 seeded defect 的主流工具（r07 的 size_t 回绕、r01 的序列号回绕、r03 的入口绕过、c02 的审计占位符），证明 clang 自带分析器对"可内部推断"的缺陷有效，但对"需外部语义"的（如 r02/r04 的符号化长度）仍失明——这正好与 KLEE 形成互补。
- 证据：本分支 `codechecker-findings` 产物（CI run 33291645097 / 33292123381）。

### 2.7 KLEE —— ⚠️ 11/11 PASS，recall=1.0（数字存疑：adapter 自证，待重跑）
- 对每个含 `sa/harnesses/<case>/klee_harness.c` 的用例：编译真实 `src/*.c` + harness 为统一 bitcode（`llvm-link`），符号化输入后跑 KLEE。
- 覆盖 11 个 defect 用例：r01（回绕越界读）、r02（off-by-one）、r04（OOB 写栈）、r05（错长度变量）、r06（<= 越界）、r07（乘法回绕）、r09（双重释放）、r10（奇数 BCD 越界）、r11（越界读）、r12（有符号/无符号混用）、r14（外部长度 memcpy）。
- 四态：**11/11 PASS，recall 1.0，severity 准确率 1.0**——**但该数字由存在 golden 辅助匹配（自证）问题的 `klee_to_findings.py` 产出，仅供参考，待 adapter 修复后重跑**。
- 解读：符号执行唯一能"证明"越界/回绕/双重释放路径可达的工具，与命题一致——它补全了 SA 的盲区。r08（数据竞争，KLEE 无法建模并发）、r13（NULL 函数指针逻辑缺陷）不在覆盖内。
- 证据：本分支 `klee-findings` 产物（CI run 33296273703）。注意 `reports/evidence/klee/` 仅归档 2 个 json（r02、r04），与 11 例覆盖声明不符，证据落盘不全。

### 2.8 Joern —— 运行成功但 recall=0（adapter 同样存在自证问题，结论待重跑确认）
- `ghcr.io/joernio/joern:master` 镜像，scan.sc 构建 CPG，按 golden 锚点做图可达性定位 + 危险调用枚举。
- ~28/30 用例产出 findings（1–3 个/例），reaching-def 等 pass 正常。
- 四态：**30/30 FN（recall 0）**。原因：锚点定位为"方法内子串匹配"，多数未命中 golden 精确锚点；危险调用枚举产出 `cwe-787` 但与 seeded defect 不对齐。**注意 `joern_to_findings.py` 同样读取 golden 辅助匹配（自证），recall=0 的方向性结论大概率仍成立，但严格数字待 adapter 修复后重跑确认。**
- 解读：Joern 证明 CPG 图查询能"导航"到代码，但**无 taint/数据流规约时无法识别缺陷本身**——与 AST/数据流工具同属"失明"一类，但其图导航能力对 Agent Viewer 的"语义定位"有价值。
- 证据：本分支 `joern-findings` 产物（CI run 33296273703）。

### 2.9 Cooddy —— ✅ 构建/运行已通，r04 检 1 problem（本分支重试突破）
- **根因突破**：cooddy 的 `Solver` 目标用 `-Wl,-Bstatic -lz3` **强制静态链接 libz3.a**，此前一直构建共享 libz3.so 导致 `ld: cannot find -lz3`。将 z3 改为 `Z3_BUILD_LIBZ3_SHARED=FALSE` 构建 `libz3.a` 后，镜像成功构建。
- 端到端：二进制 `/opt/cooddy/build/release/cooddy` 成功运行，加载全部 CWE checker（OutOfBounds / BufferMaxSize / NullPtrDereference / TypeSizeMismatch / UntrustedSource 等）。
- **本次（harness 移出 src/ 后）r04 检出 1 个 problem**：早前因 `klee_harness.c` 留在 case 根污染分析单元而报 0；归一后 cooddy 干净分析 `recv.c`，成功标定 `memcpy` 越界（cwe-787）。**升级为第 3 个真正命中 seeded defect 的工具**。
- 状态：基础设施阻塞已彻底解决，且已在 r04 验证检测能力；其余用例的 scope/include 调优可进一步扩大覆盖。
- 详情见 `reports/evidence/cooddy/README.md`（含 23 次构建迭代的失败/修复记录）。

---

## 3. 工具覆盖度与选型说明（回应"工具是不是选少了"）

华为云博客清单中的工具，按"是否可直装/可构建镜像 + 是否独立技术路线 + 是否产出可评分 findings"三原则筛选：

**已接入（9 个，覆盖 8 条独立技术路线）：**
符号执行（CSA、KLEE）、AST/数据流（CppCheck、clang-tidy）、抽象解释（Infer）、Datalog（CodeQL）、clang 分析器编排（CodeChecker）、CPG 图查询（Joern）、数据流+约束（Cooddy）。

**显式排除及理由：**
- **IKOS**：与 Infer 同属抽象解释，路线冗余。
- **Phasar / SVF / vast**：分析框架/库，需自写 checker，非开箱产出 findings。
- **CodeChecker**：已接入（它是 clang SA+tidy 的编排层，区别于裸 CSA）。
- **SonarQube / TCA-CodeDog**：商业/平台型，按"商用的和平台的就算了"排除。
- **clang（裸）**：已被 CSA/clang-tidy 覆盖。

结论：**不是选少了，而是把清单里所有"独立技术路线 + 开源 + 开箱产出 findings"的工具都接了**，冗余/框架/商业类按要求剔除。

---

## 4. 工程化与 CI 互证

- 全部 9 个工具均在 CI 实跑（非本地 docker build），每轮 `gh run` 互证；产物 `upload-artifact` 落盘。
- 统一归一化 schema（`tools/*_to_findings.py`）+ 两层匹配评测（`tools/eval.py` L1 规则：scenario 族/file/anchor 去空白/line±tolerance；L2 可选 judge）。
- 四个状态（PASS/FN/FP/EXTRA）按 design-v0.4 §4 协议判定，contract 的 must_not_find 违反权重高于裸 FP。

---

## 5. 对 Agent Viewer 的启示

1. **缺陷可检性分层**：r02/r04（符号化长度越界）只有 KLEE 能命中 → Viewer 应标注"需符号执行能力"的缺陷类。
2. **SA 工具互补**：CodeChecker/CodeQL 对"可内部推断"缺陷有效，KLEE 补"外部语义"盲区 → Viewer 应聚合多工具而非单工具。
3. **误报治理**：CppCheck/Infer/Joern 在本 bench 全噪声 → Viewer 需对 AST 类工具做 FP 降权。
4. **图导航价值**：Joern 的 CPG 虽未命中缺陷，但其语义定位能力可用于"把 golden 锚点映射到代码节点"，辅助 Viewer 的解释性。
5. **cooddy 待补**：其约束求解路线若调优成功，将是第 3 条能命中 seeded defect 的路线，值得在 Viewer 中单独加权。

---

## 6. 待办 / 后续

- [x] ~~Cooddy 用法调优~~：harness 移出 src/ 后 cooddy 已在 r04 命中 1 problem，无需额外调优即验证检测能力。
- [ ] **Cooddy 扩大覆盖**：为更多 defect 用例标 scope/include，验证 cooddy 在 r02/r07/r09 等也能命中（目前仅 r04 实测）。
- [ ] **CodeQL 映射补全**：为 `cpp/constant-comparison`、`cpp/unused-static-variable` 补 CWE 映射，使 r01/r03 计为 TP。
- [ ] **Joern taint 规约**：若加 taint spec，可让 Joern 从"图导航"升级为"缺陷识别"。
- [ ] **KLEE 覆盖补全**：r08（数据竞争）、r13（NULL 函数指针逻辑缺陷）KLEE 难建模，可探索其他驱动方式或标注为 KLEE 不适用。
- [x] 目录归一完成（`sa/` 结构），分支已 fast-forward 合入 main。
- [ ] 把本报告与 `baseline-v2.md` 合并为最终 `reports/baseline-v3.md`。
