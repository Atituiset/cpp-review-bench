# auto-linux-7d99c05ffb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#efb44d93a620c294c049db37c810c8ab7ee1122d](https://github.com/torvalds/linux/commit/efb44d93a620c294c049db37c810c8ab7ee1122d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 279 |
| 编译错误数（gcc syntax-only） | 312（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #efb44d93a620c294c049db37c810c8ab7ee1122d (https://github.com/torvalds/linux/commit/efb44d93a620c294c049db37c810c8ab7ee1122d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 2119；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT efb44d93a620c294c049db37c810c8ab7ee1122d Merge tag 'x86-urgent-2026-09-27' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2108,6 +2108,9 @@ bool filter_mce(struct mce *m)
 static __always_inline void exc_machine_check_kernel(struct pt_regs *regs)
 {
 	irqentry_state_t irq_state;
+	unsigned long dr7;
+
+	dr7 = local_db_save();
 
 	WARN_ON_ONCE(user_mode(regs));
 
@@ -2116,20 +2119,26 @@ static __always_inline void exc_machine_check_kernel(struct pt_regs *regs)
 	 * mce_check_crashing_cpu() for details.
 	 */
 	if (mca_cfg.initialized && mce_check_crashing_cpu())
-		return;
+		goto out;
 
 	irq_state = irqentry_nmi_enter(regs);
 
 	do_machine_check(regs);
 
 	irqentry_nmi_exit(regs, irq_state);
+out:
+	local_db_restore(dr7);
 }
 
 static __always_inline void exc_machine_check_user(struct pt_regs *regs)
 {
+	unsigned long dr7;
+
 	irqentry_enter_from_user_mode(regs);
 
+	dr7 = local_db_save();
 	do_machine_check(regs);
+	local_db_restore(dr7);
 
 	irqentry_exit_to_user_mode(regs);
 }
@@ -2138,21 +2147,13 @@ static __always_inline void exc_machine_check_user(struct pt_regs *regs)
 /* MCE hit kernel mode */
 DEFINE_IDTENTRY_MCE(exc_machine_check)
 {
-	unsigned long dr7;
-
-	dr7 = local_db_save();
 	exc_machine_check_kernel(regs);
-	local_db_restore(dr7);
 }
 
 /* The user mode variant. */
 DEFINE_IDTENTRY_MCE_USER(exc_machine_check)
 {
-	unsigned long dr7;
-
-	dr7 = local_db_save();
 	exc_machine_check_user(regs);
-	local_db_restore(dr7);
 }
 
 #ifdef CONFIG_X86_FRED
@@ -2169,28 +2170,20 @@ DEFINE_IDTENTRY_MCE_USER(exc_machine_check)
  */
 DEFINE_FREDENTRY_MCE(exc_machine_check)
 {
-	unsigned long dr7;
-
-	dr7 = local_db_save();
 	if (user_mode(regs))
 		exc_machine_check_user(regs);
 	else
 		exc_machine_check_kernel(regs);
-	local_db_restore(dr7);
 }
 #endif
 #else
 /* 32bit unified entry point */
 DEFINE_IDTENTRY_RAW(exc_machine_check)
 {
-	unsigned long dr7;
-
-	dr7 = local_db_save();
 	if (user_mode(regs))
 		exc_machine_check_user(regs);
 	else
 		exc_machine_check_kernel(regs);
-	local_db_restore(dr7);
 }
 #endif
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_ASM_EXTABLE_FAULT`
- 外部函数：`_ASM_EXTABLE_TYPE`
- 外部函数：`__ktime_get_real_seconds`
- 外部函数：`__this_cpu_read`
- 外部函数：`add_taint`
- 外部函数：`amd_mce_usable_address`
- 外部函数：`apei_write_mce`
- 外部函数：`arch___clear_bit`
- 外部函数：`arch___set_bit`
- 外部函数：`arch_cpu_is_offline`
- 外部函数：`arch_cpumask_clear_cpu`
- 外部函数：`arch_test_bit`
- 外部函数：`atomic_inc`
- 外部函数：`atomic_inc_return`
- 外部函数：`atomic_read`
- 外部函数：`atomic_set`
- 外部函数：`barrier`
- 外部函数：`boot_cpu_has_bug`
- 外部函数：`broadcast`
- 外部函数：`bust_spinlocks`
- 外部函数：`console_verbose`
- 外部函数：`container_of`
- 外部函数：`cpu_data`
- 外部函数：`cpuid_eax`
- 外部函数：`cpumask_and`
- 外部函数：`cpumask_pr_args`
- 外部函数：`cpumask_setall`
- 外部函数：`fixup_exception`
- 外部函数：`force_sig`
- 外部函数：`hwerr_log_error_type`
- 外部函数：`hwpoison_filter`
- 外部函数：`inc_irq_stat`
- 外部函数：`instrumentation_begin`
- 外部函数：`instrumentation_end`
- 外部函数：`int18`
- 外部函数：`intel_mce_usable_address`
- 外部函数：`irq_work_queue`
- 外部函数：`irqentry_enter_from_user_mode`
- 外部函数：`irqentry_exit_to_user_mode`
- 外部函数：`irqentry_nmi_enter`
- 外部函数：`irqentry_nmi_exit`
- 外部函数：`kexec_crash_loaded`
- 外部函数：`local_db_restore`
- 外部函数：`local_db_save`
- 外部函数：`local_irq_enable`
- 外部函数：`machine_check_poll`
- 外部函数：`mca_msr_reg`
- 外部函数：`mce_cmp`
- 外部函数：`mce_gen_pool_add`
- 外部函数：`mce_gen_pool_prepare_records`
- 外部函数：`memset`
- 外部函数：`native_rdmsrq`
- 外部函数：`native_wrmsrq`
- 外部函数：`ndelay`
- 外部函数：`num_online_cpus`
- 外部函数：`offsetof`
- 外部函数：`on_thread_stack`
- 外部函数：`panic`
- 外部函数：`pentium_machine_check`
- 外部函数：`per_cpu`
- 外部函数：`pfn_to_online_page`
- 外部函数：`pr_cont`
- 外部函数：`pr_emerg`
- 外部函数：`pr_emerg_ratelimited`
- 外部函数：`pr_err`
- 外部函数：`pr_err_once`
- 外部函数：`pr_info`
- 外部函数：`preempt_disable`
- 外部函数：`raw_atomic_add`
- 外部函数：`raw_atomic_inc_return`
- 外部函数：`raw_atomic_read`
- 外部函数：`raw_atomic_set`
- 外部函数：`rdtsc`
- 外部函数：`recover`
- 外部函数：`rmb`
- 外部函数：`set_mce_nospec`
- 外部函数：`smca_extract_err_addr`
- 外部函数：`smp_processor_id`
- 外部函数：`smp_rmb`
- 外部函数：`sync_core`
- 外部函数：`task_work`
- 外部函数：`task_work_add`
- 外部函数：`tdx_dump_mce_info`
- 外部函数：`this_cpu_ptr`
- 外部函数：`this_cpu_read`
- 外部函数：`topology_physical_package_id`
- 外部函数：`topology_ppin`
- 外部函数：`touch_nmi_watchdog`
- 外部函数：`udelay`
- 外部函数：`unlikely`
- 外部函数：`user_mode`
- 外部函数：`v8086_mode`
- 外部函数：`void`
- 外部函数：`volatile`
- 外部函数：`winchip_machine_check`
- 大写宏：`ACTION_REQUIRED`
- 大写宏：`ADDR`
- 大写宏：`AMD`
- 大写宏：`APIC`
- 大写宏：`BUG_ON`
- 大写宏：`CONFIG_MEMORY_FAILURE`
- 大写宏：`CONFIG_X86_FRED`
- 大写宏：`CPU`
- 大写宏：`DECLARE_BITMAP`
- 大写宏：`DEFINE_FREDENTRY_MCE`
- 大写宏：`DEFINE_IDTENTRY_MCE`
- 大写宏：`DEFINE_IDTENTRY_MCE_USER`
- 大写宏：`DEFINE_IDTENTRY_RAW`
- 大写宏：`EAX_EDX_DECLARE_ARGS`
- 大写宏：`EAX_EDX_RET`
- 大写宏：`EAX_EDX_VAL`
- 大写宏：`EHWPOISON`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`EX_TYPE_RDMSR_IN_MCE`
- 大写宏：`EX_TYPE_WRMSR_IN_MCE`
- 大写宏：`HWERR_RECOV_OTHERS`
- 大写宏：`HW_ERR`
- 大写宏：`INEXACT`
- 大写宏：`IPID`
- 大写宏：`LOCKDEP_NOW_UNRELIABLE`
- 大写宏：`MAX_NR_BANKS`
- 大写宏：`MCA`
- 大写宏：`MCACOD`
- 大写宏：`MCACOD_INSTR`
- 大写宏：`MCA_ADDR`
- 大写宏：`MCA_MISC`
- 大写宏：`MCA_STATUS`
- 大写宏：`MCE`
- 大写宏：`MCE_AR_SEVERITY`
- 大写宏：`MCE_CHECK_DFR_REGS`
- 大写宏：`MCE_EXCEPTION`
- 大写宏：`MCE_IN_KERNEL_COPYIN`
- 大写宏：`MCE_IN_KERNEL_RECOV`
- 大写宏：`MCE_KEEP_SEVERITY`
- 大写宏：`MCE_NO_SEVERITY`
- 大写宏：`MCE_PANIC_SEVERITY`
- 大写宏：`MCE_UCNA_SEVERITY`
- 大写宏：`MCG_STATUS_EIPV`
- 大写宏：`MCG_STATUS_LMCES`
- 大写宏：`MCG_STATUS_MCIP`
- 大写宏：`MCG_STATUS_RIPV`
- 大写宏：`MCG_STATUS_SEAM_NR`
- 大写宏：`MCI_ADDR_PHYSADDR`
- 大写宏：`MCI_MISC_ADDR_LSB`
- 大写宏：`MCI_STATUS_ADDRV`
- 大写宏：`MCI_STATUS_AR`
- 大写宏：`MCI_STATUS_EN`
- 大写宏：`MCI_STATUS_MISCV`
- 大写宏：`MCI_STATUS_OVER`
- 大写宏：`MCI_STATUS_PCC`
- 大写宏：`MCI_STATUS_POISON`
- 大写宏：`MCI_STATUS_S`
- 大写宏：`MCI_STATUS_SYNDV`
- 大写宏：`MCI_STATUS_UC`
- 大写宏：`MCI_STATUS_VAL`
- 大写宏：`MF_ACTION_REQUIRED`
- 大写宏：`MF_MUST_KILL`
- 大写宏：`MISC`
- 大写宏：`MSR_IA32_MCG_CAP`
- 大写宏：`MSR_IA32_MCG_STATUS`
- 大写宏：`MSR_IA32_MISC_ENABLE`
- 大写宏：`MSR_IA32_MISC_ENABLE_FAST_STRING`
- 大写宏：`NSEC_PER_USEC`
- 大写宏：`NULL`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`PPIN`
- 大写宏：`PROCESSOR`
- 大写宏：`RDMSR`
- 大写宏：`RIP`
- 大写宏：`SEAM`
- 大写宏：`SIGBUS`
- 大写宏：`SOCKET`
- 大写宏：`SYND`
- 大写宏：`SYND1`
- 大写宏：`SYND2`
- 大写宏：`TAINT_MACHINE_CHECK`
- 大写宏：`TIME`
- 大写宏：`TSC`
- 大写宏：`TWA_RESUME`
- 大写宏：`USEC_PER_SEC`
- 大写宏：`VM86`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`X86_BUG_TDX_PW_MCE`
- 大写宏：`X86_TRAP_MC`
- 大写宏：`X86_VENDOR_AMD`
- 大写宏：`X86_VENDOR_HYGON`
- 大写宏：`X86_VENDOR_INTEL`
- 大写宏：`X86_VENDOR_ZHAOXIN`
- 外部类型：`Allow`
- 外部类型：`Also`
- 外部类型：`Apply`
- 外部类型：`Assume`
- 外部类型：`Bail`
- 外部类型：`Bank`
- 外部类型：`Broadcast`
- 外部类型：`But`
- 外部类型：`CPUs`
- 外部类型：`Cache`
- 外部类型：`Cannot`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`Consecutive`
- 外部类型：`Corrected`
- 外部类型：`Die`
- 外部类型：`Do`
- 外部类型：`Don`
- 外部类型：`Enable`
- 外部类型：`Erratum`
- 外部类型：`Establish`
- 外部类型：`Exception`
- 外部类型：`Failed`
- 外部类型：`Fake`
- 外部类型：`Fatal`
- 外部类型：`Fault`
- 外部类型：`First`
- 外部类型：`Fixing`
- 外部类型：`For`
- 外部类型：`Get`
- 外部类型：`Given`
- 外部类型：`Go`
- 外部类型：`Grade`
- 外部类型：`Handle`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Intel`
- 外部类型：`It`
- 外部类型：`Kdump`
- 外部类型：`Kernel`
- 外部类型：`Leave`
- 外部类型：`Let`
- 外部类型：`Local`
- 外部类型：`Lx`
- 外部类型：`MCEs`
- 外部类型：`MSRs`
- 外部类型：`Machine`
- 外部类型：`Make`
- 外部类型：`Mark`
- 外部类型：`Mask`
- 外部类型：`Memory`
- 外部类型：`Monarch`
- 外部类型：`Must`
- 外部类型：`No`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Panic`
- 外部类型：`Panicing`
- 外部类型：`Poison`
- 外部类型：`Rebuild`
- 外部类型：`Rely`
- 外部类型：`Reset`
- 外部类型：`Run`
- 外部类型：`Same`
- 外部类型：`Saved`
- 外部类型：`Second`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`Starts`
- 外部类型：`Subject`
- 外部类型：`Ten`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Timeout`
- 外部类型：`Too`
- 外部类型：`Uncorrected`
- 外部类型：`Unexpected`
- 外部类型：`Use`
- 外部类型：`Wait`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`Zhaoxin`
- 外部类型：`callback_head`
- 外部类型：`irqentry_state_t`
- 外部类型：`llist_node`
- 外部类型：`mca_config`
- 外部类型：`mce`
- 外部类型：`mce_bank`
- 外部类型：`mce_evt_llist`
- 外部类型：`mce_hw_err`
- 外部类型：`page`
- 外部类型：`pt_regs`
- 外部类型：`task_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-7d99c05ffb` → 本草稿移入 `cases/defect/auto-linux-7d99c05ffb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
