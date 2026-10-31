# auto-linux-70fdf6521d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#dd384658c738d15783a786d3af2851e02490f171](https://github.com/torvalds/linux/commit/dd384658c738d15783a786d3af2851e02490f171) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 57 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #dd384658c738d15783a786d3af2851e02490f171 (https://github.com/torvalds/linux/commit/dd384658c738d15783a786d3af2851e02490f171)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT dd384658c738d15783a786d3af2851e02490f171 Merge tag 'perf-tools-for-v7.4-1-2026-10-30' of git://git.kernel.org/pub/scm/linux/kernel/git/perf/perf-tools-next :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -136,8 +136,12 @@
 #define X86_FEATURE_HYPERVISOR		( 4*32+31) /* "hypervisor" Running on a hypervisor */
 
 /* VIA/Cyrix/Centaur-defined CPU features, CPUID level 0xC0000001, word 5 */
+#define X86_FEATURE_SM2			( 5*32+ 0) /* "sm2" SM2 algorithm */
+#define X86_FEATURE_SM2_EN		( 5*32+ 1) /* "sm2_en" SM2 enabled */
 #define X86_FEATURE_XSTORE		( 5*32+ 2) /* "rng" RNG present (xstore) */
 #define X86_FEATURE_XSTORE_EN		( 5*32+ 3) /* "rng_en" RNG enabled */
+#define X86_FEATURE_CCS			( 5*32+ 4) /* "ccs" SM3 + SM4 instructions */
+#define X86_FEATURE_CCS_EN		( 5*32+ 5) /* "ccs_en" CCS enabled */
 #define X86_FEATURE_XCRYPT		( 5*32+ 6) /* "ace" on-CPU crypto (xcrypt) */
 #define X86_FEATURE_XCRYPT_EN		( 5*32+ 7) /* "ace_en" on-CPU crypto enabled */
 #define X86_FEATURE_ACE2		( 5*32+ 8) /* "ace2" Advanced Cryptography Engine v2 */
@@ -146,6 +150,12 @@
 #define X86_FEATURE_PHE_EN		( 5*32+11) /* "phe_en" PHE enabled */
 #define X86_FEATURE_PMM			( 5*32+12) /* "pmm" PadLock Montgomery Multiplier */
 #define X86_FEATURE_PMM_EN		( 5*32+13) /* "pmm_en" PMM enabled */
+#define X86_FEATURE_RNG2		( 5*32+22) /* "rng2" RNG v2 */
+#define X86_FEATURE_RNG2_EN		( 5*32+23) /* "rng2_en" RNG2 enabled */
+#define X86_FEATURE_PHE2		( 5*32+25) /* "phe2" PadLock Hash Engine v2 */
+#define X86_FEATURE_PHE2_EN		( 5*32+26) /* "phe2_en" PHE2 enabled */
+#define X86_FEATURE_RSA			( 5*32+27) /* "rsa" Big-number arithmetic */
+#define X86_FEATURE_RSA_EN		( 5*32+28) /* "rsa_en" RSA enabled */
 
 /* More extended AMD flags: CPUID level 0x80000001, ECX, word 6 */
 #define X86_FEATURE_LAHF_LM		( 6*32+ 0) /* "lahf_lm" LAHF/SAHF in long mode */
@@ -365,6 +375,8 @@
 #define X86_FEATURE_HWP_HIGHEST_PERF_CHANGE (14*32+15) /* HWP Highest perf change */
 #define X86_FEATURE_HFI			(14*32+19) /* "hfi" Hardware Feedback Interface */
 
+#define X86_FEATURE_DPTI		(14*32+24) /* Intel Directed Package Thermal Interrupt */
+
 /* AMD SVM Feature Identification, CPUID level 0x8000000a (EDX), word 15 */
 #define X86_FEATURE_NPT			(15*32+ 0) /* "npt" Nested Page Table support */
 #define X86_FEATURE_LBRV		(15*32+ 1) /* "lbrv" LBR Virtualization support */
@@ -465,6 +477,8 @@
 #define X86_FEATURE_NO_NESTED_DATA_BP	(20*32+ 0) /* No Nested Data Breakpoints */
 #define X86_FEATURE_WRMSR_XX_BASE_NS	(20*32+ 1) /* WRMSR to {FS,GS,KERNEL_GS}_BASE is non-serializing */
 #define X86_FEATURE_LFENCE_RDTSC	(20*32+ 2) /* LFENCE always serializing / synchronizes RDTSC */
+
+#define X86_FEATURE_TLBSYNC_SFW_NO	(20*32+ 4) /* TLBI Erratum #1718 fixed in microcode */
 #define X86_FEATURE_VERW_CLEAR		(20*32+ 5) /* The memory form of VERW mitigates TSA */
 #define X86_FEATURE_NULL_SEL_CLR_BASE	(20*32+ 6) /* Null Selector Clears Base */
 
@@ -575,4 +589,5 @@
 #define X86_BUG_TSA			X86_BUG( 1*32+ 9) /* "tsa" CPU is affected by Transient Scheduler Attacks */
 #define X86_BUG_VMSCAPE			X86_BUG( 1*32+10) /* "vmscape" CPU is affected by VMSCAPE attacks from guests */
 #define X86_BUG_SEAMRET_INVD_VMCS	X86_BUG( 1*32+11) /* "seamret_invd_vmcs" SEAMRET from P-SEAMLDR clears the current VMCS */
+#define X86_BUG_TLBI			X86_BUG( 1*32+12) /* "tlbi" AMD TLBI Erratum #1718 */
 #endif /* _ASM_X86_CPUFEATURES_H */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`crypto`
- 外部函数：`present`
- 大写宏：`AMD`
- 大写宏：`CPU`
- 大写宏：`CPUID`
- 大写宏：`ECX`
- 大写宏：`EDX`
- 大写宏：`HWP`
- 大写宏：`KERNEL_GS`
- 大写宏：`LAHF`
- 大写宏：`LBR`
- 大写宏：`LFENCE`
- 大写宏：`PHE`
- 大写宏：`PMM`
- 大写宏：`RDTSC`
- 大写宏：`RNG`
- 大写宏：`SAHF`
- 大写宏：`SEAMLDR`
- 大写宏：`SEAMRET`
- 大写宏：`SVM`
- 大写宏：`TSA`
- 大写宏：`VERW`
- 大写宏：`VIA`
- 大写宏：`VMCS`
- 大写宏：`VMSCAPE`
- 大写宏：`WRMSR`
- 外部类型：`Advanced`
- 外部类型：`Attacks`
- 外部类型：`Base`
- 外部类型：`Breakpoints`
- 外部类型：`Centaur`
- 外部类型：`Clears`
- 外部类型：`Cryptography`
- 外部类型：`Cyrix`
- 外部类型：`Data`
- 外部类型：`Engine`
- 外部类型：`Feature`
- 外部类型：`Feedback`
- 外部类型：`Hardware`
- 外部类型：`Highest`
- 外部类型：`Identification`
- 外部类型：`Interface`
- 外部类型：`Montgomery`
- 外部类型：`More`
- 外部类型：`Multiplier`
- 外部类型：`Nested`
- 外部类型：`No`
- 外部类型：`Null`
- 外部类型：`PadLock`
- 外部类型：`Page`
- 外部类型：`Running`
- 外部类型：`Scheduler`
- 外部类型：`Selector`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`Transient`
- 外部类型：`Virtualization`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-70fdf6521d` → 本草稿移入 `cases/defect/auto-linux-70fdf6521d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
