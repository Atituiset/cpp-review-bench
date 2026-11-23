# auto-linux-150599abe3 🟢 零依赖候选

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#6e4122f0242a3262512ee3e6f2e7e6de0bd5792b](https://github.com/torvalds/linux/commit/6e4122f0242a3262512ee3e6f2e7e6de0bd5792b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 44 |
| 编译错误数（gcc syntax-only） | 0（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6e4122f0242a3262512ee3e6f2e7e6de0bd5792b (https://github.com/torvalds/linux/commit/6e4122f0242a3262512ee3e6f2e7e6de0bd5792b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6e4122f0242a3262512ee3e6f2e7e6de0bd5792b Merge tag 'perf-tools-fixes-for-v7.4-1-2026-11-22' of git://git.kernel.org/pub/scm/linux/kernel/git/perf/perf-tools :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -76,7 +76,7 @@
 #define X86_FEATURE_K8			( 3*32+ 4) /* Opteron, Athlon64 */
 #define X86_FEATURE_ZEN5		( 3*32+ 5) /* CPU based on Zen5 microarchitecture */
 #define X86_FEATURE_ZEN6		( 3*32+ 6) /* CPU based on Zen6 microarchitecture */
-/* Free                                 ( 3*32+ 7) */
+#define X86_FEATURE_RMPOPT		( 3*32+ 7) /* Support for AMD RMPOPT instruction */
 #define X86_FEATURE_CONSTANT_TSC	( 3*32+ 8) /* "constant_tsc" TSC ticks at a constant rate */
 /* free: was #define X86_FEATURE_UP	( 3*32+ 9) * "up" SMP kernel running on UP */
 #define X86_FEATURE_ART			( 3*32+10) /* "art" Always running timer (ART) */
@@ -238,6 +238,7 @@
 // free: was #define X86_FEATURE_PVUNLOCK               ( 8*32+20) /* PV unlock function */
 #define X86_FEATURE_VCPUPREEMPT		( 8*32+21) /* PV vcpu_is_preempted function */
 #define X86_FEATURE_TDX_GUEST		( 8*32+22) /* "tdx_guest" Intel Trust Domain Extensions Guest */
+#define X86_FEATURE_PML			( 8*32+23) /* AMD Page Modification logging */
 
 /* Intel-defined CPU features, CPUID level 0x00000007:0 (EBX), word 9 */
 #define X86_FEATURE_FSGSBASE		( 9*32+ 0) /* "fsgsbase" RDFSBASE, WRFSBASE, RDGSBASE, WRGSBASE instructions*/
@@ -430,6 +431,7 @@
 #define X86_FEATURE_SUCCOR		(17*32+ 1) /* "succor" Uncorrectable error containment and recovery */
 #define X86_FEATURE_CPPC_PERF_PRIO	(17*32+ 2) /* CPPC Floor Perf support */
 #define X86_FEATURE_SMCA		(17*32+ 3) /* "smca" Scalable MCA */
+#define X86_FEATURE_BTB_CTX_ISOLATION	(17*32+ 4) /* AMD: Branch predictions contexts isolated */
 
 /* Intel-defined CPU features, CPUID level 0x00000007:0 (EDX), word 18 */
 #define X86_FEATURE_AVX512_4VNNIW	(18*32+ 2) /* "avx512_4vnniw" AVX-512 Neural Network Instructions */
@@ -485,6 +487,8 @@
 #define X86_FEATURE_AUTOIBRS		(20*32+ 8) /* Automatic IBRS */
 #define X86_FEATURE_NO_SMM_CTL_MSR	(20*32+ 9) /* SMM_CTL MSR is not present */
 
+#define X86_FEATURE_L2_TLB_SIZE_X32	(20*32+14) /* L2 TLB sizes are encoded as multiples of 32 */
+
 #define X86_FEATURE_GP_ON_USER_CPUID	(20*32+17) /* User CPUID faulting */
 
 #define X86_FEATURE_PREFETCHI		(20*32+20) /* Prefetch Data/Instruction to Cache Level */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`timer`
- 大写宏：`ART`
- 大写宏：`AVX`
- 大写宏：`CPPC`
- 大写宏：`CPU`
- 大写宏：`CPUID`
- 大写宏：`EBX`
- 大写宏：`EDX`
- 大写宏：`IBRS`
- 大写宏：`MCA`
- 大写宏：`MSR`
- 大写宏：`RDFSBASE`
- 大写宏：`RDGSBASE`
- 大写宏：`SMM_CTL`
- 大写宏：`SMP`
- 大写宏：`TSC`
- 大写宏：`WRFSBASE`
- 大写宏：`WRGSBASE`
- 大写宏：`X86_FEATURE_PVUNLOCK`
- 大写宏：`X86_FEATURE_UP`
- 外部类型：`Always`
- 外部类型：`Athlon64`
- 外部类型：`Automatic`
- 外部类型：`Cache`
- 外部类型：`Data`
- 外部类型：`Domain`
- 外部类型：`Extensions`
- 外部类型：`Floor`
- 外部类型：`Guest`
- 外部类型：`Instruction`
- 外部类型：`Instructions`
- 外部类型：`Intel`
- 外部类型：`Level`
- 外部类型：`Network`
- 外部类型：`Neural`
- 外部类型：`Opteron`
- 外部类型：`Perf`
- 外部类型：`Prefetch`
- 外部类型：`Scalable`
- 外部类型：`Trust`
- 外部类型：`Uncorrectable`
- 外部类型：`User`
- 外部类型：`Zen5`
- 外部类型：`Zen6`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-150599abe3` → 本草稿移入 `cases/defect/auto-linux-150599abe3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
