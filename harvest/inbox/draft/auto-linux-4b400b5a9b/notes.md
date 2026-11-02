# auto-linux-4b400b5a9b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9](https://github.com/torvalds/linux/commit/7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 114 |
| 编译错误数（gcc syntax-only） | 36（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9 (https://github.com/torvalds/linux/commit/7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 432；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9 Merge tag 'timers-urgent-2026-11-01' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -429,13 +429,17 @@ void play_idle_precise(u64 duration_ns, u64 latency_ns)
 	cpuidle_use_deepest_state(latency_ns);
 
 	it.done = 0;
-	hrtimer_setup_on_stack(&it.timer, idle_inject_timer_fn, CLOCK_MONOTONIC,
-			       HRTIMER_MODE_REL_HARD);
-	hrtimer_start(&it.timer, ns_to_ktime(duration_ns),
-		      HRTIMER_MODE_REL_PINNED_HARD);
+	{
+		CLASS(hrtimer_on_stack, timer)(&it.timer, idle_inject_timer_fn,
+					       CLOCK_MONOTONIC,
+					       HRTIMER_MODE_REL_HARD);
 
-	while (!READ_ONCE(it.done))
-		do_idle();
+		hrtimer_start(timer, ns_to_ktime(duration_ns),
+			      HRTIMER_MODE_REL_PINNED_HARD);
+
+		while (!READ_ONCE(it.done))
+			do_idle();
+	}
 
 	cpuidle_use_deepest_state(0);
 	current->flags &= ~PF_IDLE;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__current_clr_polling`
- 外部函数：`__current_set_polling`
- 外部函数：`container_of`
- 外部函数：`cpu_is_offline`
- 外部函数：`cpu_relax`
- 外部函数：`cpu_wakeup_latency_qos_limit`
- 外部函数：`cpuhp_report_idle_dead`
- 外部函数：`cpuidle_enter`
- 外部函数：`cpuidle_enter_s2idle`
- 外部函数：`cpuidle_find_deepest_state`
- 外部函数：`cpuidle_get_cpu_driver`
- 外部函数：`cpuidle_get_device`
- 外部函数：`cpuidle_not_available`
- 外部函数：`cpuidle_reflect`
- 外部函数：`cpuidle_select`
- 外部函数：`cpuidle_use_deepest_state`
- 外部函数：`ct_cpuidle_enter`
- 外部函数：`ct_cpuidle_exit`
- 外部函数：`current_clr_polling_and_test`
- 外部函数：`flush_smp_call_function_queue`
- 外部函数：`hrtimer_setup_on_stack`
- 外部函数：`hrtimer_start`
- 外部函数：`idle_should_enter_s2idle`
- 外部函数：`instrumentation_begin`
- 外部函数：`instrumentation_end`
- 外部函数：`interrupts`
- 外部函数：`irqs_disabled`
- 外部函数：`klp_patch_pending`
- 外部函数：`klp_update_patch_state`
- 外部函数：`local_irq_disable`
- 外部函数：`local_irq_enable`
- 外部函数：`mwait`
- 外部函数：`need_resched`
- 外部函数：`nohz_run_idle_balance`
- 外部函数：`ns_to_ktime`
- 外部函数：`preempt_set_need_resched`
- 外部函数：`raw_local_irq_disable`
- 外部函数：`raw_local_irq_enable`
- 外部函数：`rcu_nocb_flush_deferred_wakeup`
- 外部函数：`sched_ttwu_pending`
- 外部函数：`schedule_idle`
- 外部函数：`scheduled`
- 外部函数：`set_tsk_need_resched`
- 外部函数：`smp_mb__after_atomic`
- 外部函数：`smp_processor_id`
- 外部函数：`start_critical_timings`
- 外部函数：`state`
- 外部函数：`static_branch_unlikely`
- 外部函数：`stop_critical_timings`
- 外部函数：`tick_broadcast_enter`
- 外部函数：`tick_broadcast_exit`
- 外部函数：`tick_check_broadcast_expired`
- 外部函数：`tick_nohz_idle_enter`
- 外部函数：`tick_nohz_idle_exit`
- 外部函数：`tick_nohz_idle_got_tick`
- 外部函数：`tick_nohz_idle_restart_tick`
- 外部函数：`tick_nohz_idle_retain_tick`
- 外部函数：`tick_nohz_idle_stop_tick`
- 外部函数：`tick_nohz_tick_stopped`
- 外部函数：`tif_need_resched`
- 外部函数：`trace_cpu_idle`
- 外部函数：`unlikely`
- 大写宏：`CLOCK_MONOTONIC`
- 大写宏：`CPU`
- 大写宏：`EBUSY`
- 大写宏：`FAST`
- 大写宏：`FORWARD`
- 大写宏：`HRTIMER_MODE_REL_HARD`
- 大写宏：`HRTIMER_MODE_REL_PINNED_HARD`
- 大写宏：`HRTIMER_NORESTART`
- 大写宏：`INTERRUPT`
- 大写宏：`IPI`
- 大写宏：`IRQ`
- 大写宏：`NOT`
- 大写宏：`NSEC_PER_USEC`
- 大写宏：`PENDING`
- 大写宏：`PF_IDLE`
- 大写宏：`PREEMPT_NEED_RESCHED`
- 大写宏：`PWR_EVENT_EXIT`
- 大写宏：`RCU`
- 大写宏：`READ_ONCE`
- 大写宏：`ROLLBACK`
- 大写宏：`SLEEP`
- 大写宏：`TIF_NEED_RESCHED`
- 大写宏：`UNTIL`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WRITE_ONCE`
- 外部类型：`All`
- 外部类型：`Also`
- 外部类型：`And`
- 外部类型：`Ask`
- 外部类型：`Check`
- 外部类型：`Enter`
- 外部类型：`Give`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Interrupts`
- 外部类型：`It`
- 外部类型：`Otherwise`
- 外部类型：`Our`
- 外部类型：`Possibly`
- 外部类型：`Several`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`Suspend`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`We`
- 外部类型：`cpuidle_device`
- 外部类型：`cpuidle_driver`
- 外部类型：`hrtimer`
- 外部类型：`hrtimer_restart`
- 外部类型：`idle_timer`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-4b400b5a9b` → 本草稿移入 `cases/defect/auto-linux-4b400b5a9b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
