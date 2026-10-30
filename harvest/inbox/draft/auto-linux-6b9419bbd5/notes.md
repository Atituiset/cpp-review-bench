# auto-linux-6b9419bbd5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9c7e3d59107e2daf204bf6bf87b6c448ca89defe](https://github.com/torvalds/linux/commit/9c7e3d59107e2daf204bf6bf87b6c448ca89defe) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 148 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9c7e3d59107e2daf204bf6bf87b6c448ca89defe (https://github.com/torvalds/linux/commit/9c7e3d59107e2daf204bf6bf87b6c448ca89defe)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2576；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9c7e3d59107e2daf204bf6bf87b6c448ca89defe Merge tag 'hyperv-next-signed-20261028' of git://git.kernel.org/pub/scm/linux/kernel/git/hyperv/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -33,6 +33,7 @@
 #include <linux/sysfb.h>
 #include <linux/efi.h>
 #include <linux/kernel.h>
+#include <linux/kexec.h>
 #include <linux/syscore_ops.h>
 #include <linux/dma-map-ops.h>
 #include <linux/pci.h>
@@ -2571,10 +2572,14 @@ EXPORT_SYMBOL_GPL(vmbus_free_mmio);
 #ifdef CONFIG_ACPI
 static int vmbus_acpi_add(struct platform_device *pdev)
 {
+	struct acpi_device *ancestor;
+	struct acpi_device *device;
 	acpi_status result;
 	int ret_val = -ENODEV;
-	struct acpi_device *ancestor;
-	struct acpi_device *device = ACPI_COMPANION(&pdev->dev);
+
+	device = ACPI_COMPANION(&pdev->dev);
+	if (!device)
+		return -ENODEV;
 
 	vmbus_root_device = &device->dev;
 
@@ -2904,6 +2909,12 @@ static void hv_kexec_handler(void)
 	cpuhp_remove_state(hyperv_cpuhp_online);
 };
 
+static void hv_vmbus_syscore_shutdown(void *data)
+{
+	if (kexec_is_in_progress())
+		hv_kexec_handler();
+}
+
 static void hv_crash_handler(struct pt_regs *regs)
 {
 	int cpu;
@@ -2954,14 +2965,15 @@ static void hv_synic_resume(void *data)
 	 */
 }
 
-/* The callbacks run only on CPU0, with irqs_disabled. */
-static const struct syscore_ops hv_synic_syscore_ops = {
+/* The suspend and resume callbacks run only on CPU0, with irqs_disabled. */
+static const struct syscore_ops hv_vmbus_syscore_ops = {
 	.suspend = hv_synic_suspend,
 	.resume = hv_synic_resume,
+	.shutdown = hv_vmbus_syscore_shutdown,
 };
 
-static struct syscore hv_synic_syscore = {
-	.ops = &hv_synic_syscore_ops,
+static struct syscore hv_vmbus_syscore = {
+	.ops = &hv_vmbus_syscore_ops,
 };
 
 static int __init hv_acpi_init(void)
@@ -3010,10 +3022,9 @@ static int __init hv_acpi_init(void)
 	if (ret)
 		goto cleanup;
 
-	hv_setup_kexec_handler(hv_kexec_handler);
 	hv_setup_crash_handler(hv_crash_handler);
 
-	register_syscore(&hv_synic_syscore);
+	register_syscore(&hv_vmbus_syscore);
 
 	return 0;
 
@@ -3035,9 +3046,8 @@ static void __exit vmbus_exit(void)
 		 */
 		return;
 
-	unregister_syscore(&hv_synic_syscore);
+	unregister_syscore(&hv_vmbus_syscore);
 
-	hv_remove_kexec_handler();
 	hv_remove_crash_handler();
 	hv_stimer_global_cleanup();
 	vmbus_disconnect();
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__this_cpu_read`
- 外部函数：`__this_cpu_write`
- 外部函数：`alloc_percpu`
- 外部函数：`atomic_notifier_chain_register`
- 外部函数：`bus_register`
- 外部函数：`bus_unregister`
- 外部函数：`clockevents_shutdown`
- 外部函数：`cpuhp_remove_state`
- 外部函数：`cpuhp_setup_state_nocalls_cpuslocked`
- 外部函数：`cpus_read_lock`
- 外部函数：`cpus_read_unlock`
- 外部函数：`create_image`
- 外部函数：`flush_work`
- 外部函数：`free_percpu`
- 外部函数：`free_percpu_irq`
- 外部函数：`hv_begin_read`
- 外部函数：`hv_ce_shutdown`
- 外部函数：`hv_debug_init`
- 外部函数：`hv_hyp_synic_disable_regs`
- 外部函数：`hv_hyp_synic_enable_regs`
- 外部函数：`hv_init`
- 外部函数：`hv_is_hyperv_initialized`
- 外部函数：`hv_para_set_sint_proxy`
- 外部函数：`hv_remove_crash_handler`
- 外部函数：`hv_remove_kexec_handler`
- 外部函数：`hv_remove_vmbus_handler`
- 外部函数：`hv_root_partition`
- 外部函数：`hv_setup_crash_handler`
- 外部函数：`hv_setup_kexec_handler`
- 外部函数：`hv_setup_vmbus_handler`
- 外部函数：`hv_stimer_cleanup`
- 外部函数：`hv_stimer_global_cleanup`
- 外部函数：`hv_stimer_init`
- 外部函数：`hv_synic_alloc`
- 外部函数：`hv_synic_free`
- 外部函数：`hv_synic_init`
- 外部函数：`kfree_rcu`
- 外部函数：`lock_map_acquire_try`
- 外部函数：`lock_map_release`
- 外部函数：`mb`
- 外部函数：`per_cpu_ptr`
- 外部函数：`platform_driver_register`
- 外部函数：`platform_driver_unregister`
- 外部函数：`pr_err`
- 外部函数：`pr_info`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`register_syscore`
- 外部函数：`relid2channel`
- 外部函数：`request_percpu_irq`
- 外部函数：`resume_target_kernel`
- 外部函数：`schedule_work_on`
- 外部函数：`smp_processor_id`
- 外部函数：`smpboot_register_percpu_thread`
- 外部函数：`smpboot_unregister_percpu_thread`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`sync_test_and_clear_bit`
- 外部函数：`syscore_resume`
- 外部函数：`syscore_suspend`
- 外部函数：`tasklet_schedule`
- 外部函数：`this_cpu_ptr`
- 外部函数：`tick_suspend`
- 外部函数：`timekeeping_resume`
- 外部函数：`timekeeping_suspend`
- 外部函数：`trace_vmbus_chan_sched`
- 外部函数：`unlikely`
- 外部函数：`unregister_syscore`
- 外部函数：`vector`
- 外部函数：`vmbus_chan_release`
- 外部函数：`vmbus_connect`
- 外部函数：`vmbus_disconnect`
- 外部函数：`vmbus_initiate_unload`
- 外部函数：`vmbus_request_offers`
- 外部函数：`vmbus_reset_channel_cb`
- 外部函数：`void`
- 外部函数：`wake_up_process`
- 外部函数：`x86`
- 大写宏：`ACPI`
- 大写宏：`ACPI_COMPANION`
- 大写宏：`ARM64`
- 大写宏：`CONFIG_ACPI`
- 大写宏：`CONFIG_PREEMPT_RT`
- 大写宏：`CPU`
- 大写宏：`CPU0`
- 大写宏：`CPUHP_AP_ONLINE_DYN`
- 大写宏：`DEFINE_WAIT_OVERRIDE_MAP`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`HVMSG_NONE`
- 大写宏：`HV_CALL_BATCHED`
- 大写宏：`HV_CALL_DIRECT`
- 大写宏：`HV_CALL_ISR`
- 大写宏：`HYPERVISOR_CALLBACK_VECTOR`
- 大写宏：`INIT_WORK`
- 大写宏：`IRQ`
- 大写宏：`IRQ_HANDLED`
- 大写宏：`IS_ENABLED`
- 大写宏：`LD_WAIT_CONFIG`
- 大写宏：`NULL`
- 大写宏：`PREEMPT`
- 大写宏：`PREEMPT_RT`
- 大写宏：`READ_ONCE`
- 大写宏：`VMBUS_MESSAGE_SINT`
- 外部类型：`Always`
- 外部类型：`CPUs`
- 外部类型：`Cache`
- 外部类型：`Can`
- 外部类型：`Check`
- 外部类型：`Ensure`
- 外部类型：`Err`
- 外部类型：`Establishing`
- 外部类型：`Find`
- 外部类型：`Get`
- 外部类型：`Guarantees`
- 外部类型：`However`
- 外部类型：`Hyper`
- 外部类型：`If`
- 外部类型：`Initialize`
- 外部类型：`Linux`
- 外部类型：`Make`
- 外部类型：`Note`
- 外部类型：`On`
- 外部类型：`Pairs`
- 外部类型：`Register`
- 外部类型：`Return`
- 外部类型：`See`
- 外部类型：`Special`
- 外部类型：`SynIC`
- 外部类型：`Test`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`Therefore`
- 外部类型：`Unable`
- 外部类型：`VMBus`
- 外部类型：`VMbus`
- 外部类型：`When`
- 外部类型：`acpi_device`
- 外部类型：`hv_message`
- 外部类型：`hv_per_cpu_context`
- 外部类型：`hv_synic_event_flags`
- 外部类型：`irqreturn_t`
- 外部类型：`platform_device`
- 外部类型：`pt_regs`
- 外部类型：`spinlock_t`
- 外部类型：`syscore`
- 外部类型：`syscore_ops`
- 外部类型：`task_struct`
- 外部类型：`vmbus_channel`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-6b9419bbd5` → 本草稿移入 `cases/defect/auto-linux-6b9419bbd5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
