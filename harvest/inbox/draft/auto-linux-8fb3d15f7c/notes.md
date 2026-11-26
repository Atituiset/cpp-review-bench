# auto-linux-8fb3d15f7c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#f441d24349a3fb7f0b0f8f76f05686e448fd0b10](https://github.com/torvalds/linux/commit/f441d24349a3fb7f0b0f8f76f05686e448fd0b10) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 155 |
| 编译错误数（gcc syntax-only） | 224（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f441d24349a3fb7f0b0f8f76f05686e448fd0b10 (https://github.com/torvalds/linux/commit/f441d24349a3fb7f0b0f8f76f05686e448fd0b10)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 5037；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f441d24349a3fb7f0b0f8f76f05686e448fd0b10 Merge tag 'sched_ext-for-7.4-rc4-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/sched_ext :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5017,6 +5017,7 @@ void wake_up_new_task(struct task_struct *p)
 	struct rq_flags rf;
 	struct rq *rq;
 	int wake_flags = WF_FORK;
+	int en_flags = ENQUEUE_NOCLOCK | ENQUEUE_INITIAL;
 
 	raw_spin_lock_irqsave(&p->pi_lock, rf.flags);
 	WRITE_ONCE(p->__state, TASK_RUNNING);
@@ -5030,11 +5031,13 @@ void wake_up_new_task(struct task_struct *p)
 	 */
 	p->recent_used_cpu = task_cpu(p);
 	__set_task_cpu(p, select_task_rq(p, task_cpu(p), &wake_flags));
+	if (wake_flags & WF_RQ_SELECTED)
+		en_flags |= ENQUEUE_RQ_SELECTED;
 	rq = __task_rq_lock(p, &rf);
 	update_rq_clock(rq);
 	post_init_entity_util_avg(p);
 
-	activate_task(rq, p, ENQUEUE_NOCLOCK | ENQUEUE_INITIAL);
+	activate_task(rq, p, en_flags);
 	trace_sched_wakeup_new(p);
 	wakeup_preempt(rq, p, wake_flags);
 	if (p->sched_class->task_woken) {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__clear_task_blocked_on`
- 外部函数：`__set_task_cpu`
- 外部函数：`__task_rq_lock`
- 外部函数：`cpu_active`
- 外部函数：`cpu_dying`
- 外部函数：`cpu_of`
- 外部函数：`cpu_online`
- 外部函数：`cpu_preferred`
- 外部函数：`cpumask_any`
- 外部函数：`cpumask_intersects_and`
- 外部函数：`cpumask_of_node`
- 外部函数：`cpumask_test_cpu`
- 外部函数：`cpuset_cpus_allowed_fallback`
- 外部函数：`d`
- 外部函数：`delayacct_irq`
- 外部函数：`fetch_or`
- 外部函数：`here`
- 外部函数：`irq_time_read`
- 外部函数：`irqtime_enabled`
- 外部函数：`is_idle_task`
- 外部函数：`is_migration_disabled`
- 外部函数：`kfree`
- 外部函数：`kfree_rcu`
- 外部函数：`kthread_is_per_cpu`
- 外部函数：`lockdep_assert_held`
- 外部函数：`lockdep_assert_rq_held`
- 外部函数：`migrate_disabled`
- 外部函数：`paravirt_steal_clock`
- 外部函数：`post_init_entity_util_avg`
- 外部函数：`printk_deferred`
- 外部函数：`printk_ratelimit`
- 外部函数：`prio_less`
- 外部函数：`psi_enqueue`
- 外部函数：`raw_spin_lock_irqsave`
- 外部函数：`rb_add`
- 外部函数：`rb_entry`
- 外部函数：`rb_sched_core_less`
- 外部函数：`rq_clock_skip_update`
- 外部函数：`sched_class_above`
- 外部函数：`sched_clock_cpu`
- 外部函数：`sched_core_enabled`
- 外部函数：`sched_feat`
- 外部函数：`sched_info_enqueue`
- 外部函数：`scx_rq_clock_update`
- 外部函数：`select_task`
- 外部函数：`set_cpus_allowed`
- 外部函数：`set_preempt_need_resched`
- 外部函数：`set_task_cpu`
- 外部函数：`set_ti_thread_flag`
- 外部函数：`smp_processor_id`
- 外部函数：`smp_send_reschedule`
- 外部函数：`static_branch_unlikely`
- 外部函数：`task_allowed_on_cpu`
- 外部函数：`task_cpu`
- 外部函数：`task_cpu_fallback_mask`
- 外部函数：`task_cpu_possible_mask`
- 外部函数：`task_current`
- 外部函数：`task_on_rq_migrating`
- 外部函数：`task_on_rq_queued`
- 外部函数：`task_pid_nr`
- 外部函数：`task_rq`
- 外部函数：`task_thread_info`
- 外部函数：`test_tsk_need_resched`
- 外部函数：`trace_sched_set_need_resched_tp`
- 外部函数：`trace_sched_wake_idle_without_ipi`
- 外部函数：`trace_sched_wakeup_new`
- 外部函数：`try_cmpxchg`
- 外部函数：`ttwu`
- 外部函数：`typeof`
- 外部函数：`uclamp_is_used`
- 外部函数：`uclamp_none`
- 外部函数：`uclamp_rq_get`
- 外部函数：`uclamp_rq_set`
- 外部函数：`uclamp_se_set`
- 外部函数：`uclamp_tg_restrict`
- 外部函数：`unlikely`
- 外部函数：`update_irq_load_avg`
- 外部函数：`update_rq_clock_pelt`
- 大写宏：`ASSERT_EXCLUSIVE_WRITER`
- 大写宏：`BUG`
- 大写宏：`CONFIG_HAVE_SCHED_AVG_IRQ`
- 大写宏：`CONFIG_IRQ_TIME_ACCOUNTING`
- 大写宏：`CONFIG_PARAVIRT_TIME_ACCOUNTING`
- 大写宏：`CPU`
- 大写宏：`DEQUEUE_SAVE`
- 大写宏：`ENQUEUE_DELAYED`
- 大写宏：`ENQUEUE_INITIAL`
- 大写宏：`ENQUEUE_MIGRATED`
- 大写宏：`ENQUEUE_NOCLOCK`
- 大写宏：`ENQUEUE_RESTORE`
- 大写宏：`FAIR`
- 大写宏：`IRQ`
- 大写宏：`KTHREAD_IS_PER_CPU`
- 大写宏：`NONTASK_CAPACITY`
- 大写宏：`NOP`
- 大写宏：`NULL`
- 大写宏：`PF_KTHREAD`
- 大写宏：`PREEMPT_RT`
- 大写宏：`RQCF_ACT_SKIP`
- 大写宏：`RQCF_UPDATED`
- 大写宏：`RUNNABLE`
- 大写宏：`SCA_USER`
- 大写宏：`TASK_ON_RQ_QUEUED`
- 大写宏：`TASK_RUNNING`
- 大写宏：`TIF_NEED_RESCHED`
- 大写宏：`TIF_NEED_RESCHED_LAZY`
- 大写宏：`UCLAMP_FLAG_IDLE`
- 大写宏：`WARN_DOUBLE_CLOCK`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WF_FORK`
- 大写宏：`WF_RQ_SELECTED`
- 大写宏：`WRITE_ONCE`
- 外部类型：`Always`
- 外部类型：`Any`
- 外部类型：`Avoid`
- 外部类型：`Because`
- 外部类型：`But`
- 外部类型：`Can`
- 外部类型：`Don`
- 外部类型：`Guy`
- 外部类型：`If`
- 外部类型：`Ignore`
- 外部类型：`In`
- 外部类型：`It`
- 外部类型：`Local`
- 外部类型：`Look`
- 外部类型：`Mr`
- 外部类型：`Nice`
- 外部类型：`No`
- 外部类型：`Non`
- 外部类型：`Only`
- 外部类型：`Otherwise`
- 外部类型：`Regular`
- 外部类型：`Reset`
- 外部类型：`Since`
- 外部类型：`System`
- 外部类型：`Task`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`Try`
- 外部类型：`Update`
- 外部类型：`When`
- 外部类型：`affinity_context`
- 外部类型：`cpumask`
- 外部类型：`cpumask_t`
- 外部类型：`rb_node`
- 外部类型：`rcu_head`
- 外部类型：`rq`
- 外部类型：`rq_flags`
- 外部类型：`task_struct`
- 外部类型：`thread_info`
- 外部类型：`uclamp_bucket`
- 外部类型：`uclamp_id`
- 外部类型：`uclamp_rq`
- 外部类型：`uclamp_se`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-8fb3d15f7c` → 本草稿移入 `cases/defect/auto-linux-8fb3d15f7c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
