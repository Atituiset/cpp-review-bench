# auto-linux-bd8d5fc9e2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#97d73c8251226cf27cc6656b4c542869bb8c69fa](https://github.com/torvalds/linux/commit/97d73c8251226cf27cc6656b4c542869bb8c69fa) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #97d73c8251226cf27cc6656b4c542869bb8c69fa (https://github.com/torvalds/linux/commit/97d73c8251226cf27cc6656b4c542869bb8c69fa)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 97d73c8251226cf27cc6656b4c542869bb8c69fa Merge tag 'arm64-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/arm64/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -420,13 +420,33 @@ static const struct midr_range workaround_amu_constcnt_list[] = {
 	MIDR_RANGE(MIDR_CORTEX_A510, 0, 0, 1, 1),
 #endif
 #ifdef CONFIG_ARM64_ERRATUM_3821522
+	/* Cortex-A720 r0p0 - r0p2 */
+	MIDR_RANGE(MIDR_CORTEX_A720, 0, 0, 0, 2),
+	/* Cortex-A720AE r0p0 - r0p1 */
+	MIDR_RANGE(MIDR_CORTEX_A720AE, 0, 0, 0, 1),
 	/* Cortex-A725 r0p0 - r0p2 */
 	MIDR_RANGE(MIDR_CORTEX_A725, 0, 0, 0, 2),
+	/* Neoverse N3 r0p0 - r0p1 */
+	MIDR_RANGE(MIDR_NEOVERSE_N3, 0, 0, 0, 1),
+	/* C1-Pro r0p0 - r1p3 */
+	MIDR_RANGE(MIDR_C1_PRO, 0, 0, 1, 3),
 #endif
 	{}
 };
 #endif /* CONFIG_ARM64_WORKAROUND_BROKEN_AMU_CONSTCNT */
 
+#ifdef CONFIG_ARM64_WORKAROUND_BROKEN_AMU_CORECNT
+static const struct midr_range workaround_amu_corecnt_list[] = {
+#ifdef CONFIG_ARM64_ERRATUM_2738461
+	/* Cortex-A715 r0p0 - r1p3 */
+	MIDR_RANGE(MIDR_CORTEX_A715, 0, 0, 1, 3),
+	/* Cortex-A720 r0p0 - r0p1 */
+	MIDR_RANGE(MIDR_CORTEX_A720, 0, 0, 0, 1),
+#endif
+	{}
+};
+#endif /* CONFIG_ARM64_WORKAROUND_BROKEN_AMU_CORECNT */
+
 #ifdef CONFIG_CAVIUM_ERRATUM_23154
 static const struct midr_range cavium_erratum_23154_cpus[] = {
 	MIDR_ALL_VERSIONS(MIDR_THUNDERX),
@@ -970,6 +990,14 @@ const struct arm64_cpu_capabilities arm64_errata[] = {
 		CAP_MIDR_RANGE_LIST(workaround_amu_constcnt_list)
 	},
 #endif
+#ifdef CONFIG_ARM64_WORKAROUND_BROKEN_AMU_CORECNT
+	{
+		.desc = "Broken AMU AMEVCNTR00 (core cycle counter)",
+		.capability = ARM64_WORKAROUND_BROKEN_AMU_CORECNT,
+		.type = ARM64_CPUCAP_WEAK_LOCAL_CPU_FEATURE,
+		CAP_MIDR_RANGE_LIST(workaround_amu_corecnt_list)
+	},
+#endif
 #ifdef CONFIG_ARM64_ERRATUM_2038923
 	{
 		.desc = "ARM erratum 2038923",
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`midr_is_cpu_model_range`
- 外部函数：`preemptible`
- 外部函数：`read_cpuid_id`
- 大写宏：`A725`
- 大写宏：`ARM`
- 大写宏：`CONFIG_ARM64_ERRATUM_2038923`
- 大写宏：`CONFIG_ARM64_ERRATUM_3821522`
- 大写宏：`CONFIG_ARM64_WORKAROUND_BROKEN_AMU_CONSTCNT`
- 大写宏：`CONFIG_CAVIUM_ERRATUM_23154`
- 大写宏：`MIDR_ALL_VERSIONS`
- 大写宏：`MIDR_CORTEX_A510`
- 大写宏：`MIDR_CORTEX_A725`
- 大写宏：`MIDR_RANGE`
- 大写宏：`MIDR_THUNDERX`
- 大写宏：`SCOPE_LOCAL_CPU`
- 大写宏：`WARN_ON`
- 外部类型：`Cortex`
- 外部类型：`arm64_cpu_capabilities`
- 外部类型：`midr_range`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-bd8d5fc9e2` → 本草稿移入 `cases/defect/auto-linux-bd8d5fc9e2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
