# auto-linux-5e0188be23

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a9daf327f3ff90625204876981f88cfe2b6f6fff](https://github.com/torvalds/linux/commit/a9daf327f3ff90625204876981f88cfe2b6f6fff) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 368 |
| 编译错误数（gcc syntax-only） | 393（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a9daf327f3ff90625204876981f88cfe2b6f6fff (https://github.com/torvalds/linux/commit/a9daf327f3ff90625204876981f88cfe2b6f6fff)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 4313；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a9daf327f3ff90625204876981f88cfe2b6f6fff Merge tag 'sched_ext-for-7.4-rc2-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/sched_ext :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -461,6 +461,14 @@ static bool rq_is_open(struct rq *rq, u64 enq_flags)
 	 * @rq is either picking, in transition to, or running an SCX task.
 	 */
 
+	/*
+	 * A local insertion can raise next_class to ext while the donor is
+	 * still idle. Keep the rq open so that the local DSQ head is not
+	 * unnecessarily reenqueued before the CPU can pick it.
+	 */
+	if (rq->donor == rq->idle)
+		return true;
+
 	/*
 	 * If we're in the dispatch path holding rq lock, $donor may or may not
 	 * be ready depending on whether the on-going dispatch decides to extend
@@ -580,12 +588,14 @@ enum scx_dsq_iter_flags {
 	/* iterate in the reverse dispatch order */
 	SCX_DSQ_ITER_REV		= 1U << 16,
 
+	__SCX_DSQ_ITER_DRAINED		= 1U << 28,
 	__SCX_DSQ_ITER_HAS_REENQ	= 1U << 29,
 	__SCX_DSQ_ITER_HAS_SLICE	= 1U << 30,
 	__SCX_DSQ_ITER_HAS_VTIME	= 1U << 31,
 
 	__SCX_DSQ_ITER_USER_FLAGS	= SCX_DSQ_ITER_REV,
 	__SCX_DSQ_ITER_ALL_FLAGS	= __SCX_DSQ_ITER_USER_FLAGS |
+					  __SCX_DSQ_ITER_DRAINED |
 					  __SCX_DSQ_ITER_HAS_REENQ |
 					  __SCX_DSQ_ITER_HAS_SLICE |
 					  __SCX_DSQ_ITER_HAS_VTIME,
@@ -2225,11 +2235,11 @@ static void direct_dispatch(struct scx_sched *sch, struct task_struct *p,
 bool scx_rq_online(struct rq *rq)
 {
 	/*
-	 * Test both cpu_active() and %SCX_RQ_ONLINE. %SCX_RQ_ONLINE follows the
-	 * core's rq online state. cpu_active() test guarantees that, if this
-	 * function returns %true, %SCX_RQ_ONLINE will stay set until the
-	 * current scheduling operation is complete even if we aren't locking
-	 * @rq.
+	 * Test both cpu_active() and %SCX_RQ_ONLINE. %SCX_RQ_ONLINE is set
+	 * after the root's ops.cpu_online() and cleared when the cpu goes down.
+	 * cpu_active() test guarantees that, if this function returns %true,
+	 * %SCX_RQ_ONLINE will stay set until the current scheduling operation
+	 * is complete even if we aren't locking @rq.
 	 */
 	return likely((rq->scx.flags & SCX_RQ_ONLINE) && cpu_active(cpu_of(rq)));
 }
@@ -2420,7 +2430,7 @@ static void set_task_runnable(struct rq *rq, struct task_struct *p)
 	}
 
 	/*
-	 * list_add_tail() must be used. scx_bypass() and rq_offline_scx()
+	 * list_add_tail() must be used. scx_bypass() and drain_offline_rq()
 	 * depend on tasks being appended to the runnable_list.
 	 */
 	list_add_tail(&p->scx.runnable_node, &rq->scx.runnable_list);
@@ -3399,6 +3409,12 @@ static enum scx_dsp_verdict dispatch_one(struct rq *rq, struct task_struct *prev
 		rq->scx.cpu_released = false;
 	}
 
+	/* the ecaps sync may have dropped the rq lock */
+	if (scx_dsp_prev_stale(rq, prev)) {
+		verdict = SCX_DSP_RETRY;
+		goto has_tasks;
+	}
+
 	if (prev->sched_class == &ext_sched_class) {
 		update_curr_scx(rq);
 
@@ -3441,6 +3457,10 @@ static enum scx_dsp_verdict dispatch_one(struct rq *rq, struct task_struct *prev
 	}
 
 	verdict = scx_dispatch_sched(root_sch, rq, prev, false);
+
+	/* scx_dispatch_sched() may have dropped the rq lock */
+	if (scx_dsp_prev_stale(rq, prev))
+		verdict = SCX_DSP_RETRY;
 	if (verdict != SCX_DSP_NONE)
 		goto has_tasks;
 
@@ -4296,12 +4316,37 @@ static void set_cpus_allowed_scx(struct task_struct *p,
 		scx_call_op_set_cpumask(sch, task_rq(p), p, (struct cpumask *)p->cpus_ptr);
 }
 
+static void drain_offline_rq(struct rq *rq)
+{
+	struct task_struct *p, *n;
+
+	scx_rescue_flush(rq);
+
+	/*
+	 * An offline CPU no longer calls ops.dispatch(). Re-enqueue its tasks
+	 * onto the local DSQ so that they run here and balance_push() moves
+	 * them off.
+	 */
+	list_for_each_entry_safe_reverse(p, n, &rq->scx.runnable_list, scx.runnable_node) {
+		if (p->scx.dsq == &rq->scx.local_dsq)
+			continue;
+		guard(sched_change)(p, DEQUEUE_SAVE | DEQUEUE_MOVE | DEQUEUE_NOCLOCK);
+	}
+}
+
 static void handle_hotplug(struct rq *rq, bool online)
 {
 	struct scx_sched *sch = scx_root_protected();
 	s32 cpu = cpu_of(rq);
 	s32 cpu_or_cid = cpu;
 
+	if (!online) {
+		guard(rq_lock_irqsave)(rq);
+		update_rq_clock(rq);
+		rq->scx.flags &= ~SCX_RQ_ONLINE;
+		drain_offline_rq(rq);
+	}
+
 	atomic_long_inc(&s
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__attribute__`
- 外部函数：`__schedule`
- 外部函数：`__scx_add_event`
- 外部函数：`__scx_exit`
- 外部函数：`activate_task`
- 外部函数：`aligned`
- 外部函数：`atomic64_read`
- 外部函数：`atomic64_set`
- 外部函数：`atomic64_xchg`
- 外部函数：`atomic_long_inc`
- 外部函数：`atomic_long_read`
- 外部函数：`atomic_long_read_acquire`
- 外部函数：`atomic_long_set`
- 外部函数：`atomic_long_set_release`
- 外部函数：`atomic_read`
- 外部函数：`balance_push`
- 外部函数：`container_of`
- 外部函数：`cpu_active`
- 外部函数：`cpu_of`
- 外部函数：`cpu_possible`
- 外部函数：`cpu_relax`
- 外部函数：`cpu_rq`
- 外部函数：`cpu_to_node`
- 外部函数：`cpufreq_update_util`
- 外部函数：`cpumask_test_cpu`
- 外部函数：`deactivate_task`
- 外部函数：`dequeue_task_scx`
- 外部函数：`destroy_dsq`
- 外部函数：`dispatch_local_dsq`
- 外部函数：`dispatch_one`
- 外部函数：`dl_server_update`
- 外部函数：`do_task_dead`
- 外部函数：`dsq_dec_nr`
- 外部函数：`dsq_inc_nr`
- 外部函数：`dsq_insert_head`
- 外部函数：`enable`
- 外部函数：`enqueue`
- 外部函数：`finish_dispatch`
- 外部函数：`guard`
- 外部函数：`irq_work_sync`
- 外部函数：`is_migration_disabled`
- 外部函数：`kthread_flush_work`
- 外部函数：`likely`
- 外部函数：`list_add`
- 外部函数：`list_add_tail`
- 外部函数：`list_del_init`
- 外部函数：`list_empty`
- 外部函数：`list_first_entry_or_null`
- 外部函数：`list_for_each_entry_safe_reverse`
- 外部函数：`list_move`
- 外部函数：`list_move_tail`
- 外部函数：`lock`
- 外部函数：`lockdep_assert_held`
- 外部函数：`lockdep_assert_rq_held`
- 外部函数：`min_t`
- 外部函数：`num_possible_cpus`
- 外部函数：`ops_dequeue`
- 外部函数：`proxy_set_task_cpu`
- 外部函数：`put_prev_task_scx`
- 外部函数：`raw_smp_processor_id`
- 外部函数：`raw_spin_lock`
- 外部函数：`raw_spin_lock_nested`
- 外部函数：`raw_spin_rq_lock`
- 外部函数：`raw_spin_rq_trylock`
- 外部函数：`raw_spin_rq_unlock`
- 外部函数：`raw_spin_unlock`
- 外部函数：`rb_add`
- 外部函数：`rb_erase`
- 外部函数：`rb_prev`
- 外部函数：`rcu_access_pointer`
- 外部函数：`rcu_assign_pointer`
- 外部函数：`resched_curr`
- 外部函数：`resched_curr_lazy`
- 外部函数：`rhashtable_lookup`
- 外部函数：`rq_lock_irqsave`
- 外部函数：`rq_owned_post_enq`
- 外部函数：`rq_unlock_irqrestore`
- 外部函数：`sched_class_above`
- 外部函数：`sched_core_enabled`
- 外部函数：`sched_proxy_exec`
- 外部函数：`sched_update_tick_dependency`
- 外部函数：`sched_weight_to_cgroup`
- 外部函数：`schedule_deferred_locked`
- 外部函数：`schedule_dsq_reenq`
- 外部函数：`scx_bpf_dsq_reenq`
- 外部函数：`scx_bpf_sub_grant`
- 外部函数：`scx_bpf_sub_revoke`
- 外部函数：`scx_bypass`
- 外部函数：`scx_bypass_dsq`
- 外部函数：`scx_cid_to_cpu`
- 外部函数：`scx_cmask_ref_from_cpumask`
- 外部函数：`scx_cmask_ref_init_kern`
- 外部函数：`scx_cpu_ret`
- 外部函数：`scx_cpuperf_set`
- 外部函数：`scx_dispatch_sched`
- 外部函数：`scx_divert_strip_flags`
- 外部函数：`scx_enabled`
- 外部函数：`scx_error`
- 外部函数：`scx_exec_sched`
- 外部函数：`scx_idle_update_selcpu_topology`
- 外部函数：`scx_is_cid_type`
- 外部函数：`scx_kaddr_to_arena`
- 外部函数：`scx_kf_allowed_ctx`
- 外部函数：`scx_locked_rq`
- 外部函数：`scx_missing_caps`
- 外部函数：`scx_online_ecaps`
- 外部函数：`scx_prog_sched`
- 外部函数：`scx_rescue_charge`
- 外部函数：`scx_rescue_enabled`
- 外部函数：`scx_rescue_flush`
- 外部函数：`scx_rescuee`
- 外部函数：`scx_resolve_local_dsq`
- 外部函数：`scx_root_protected`
- 外部函数：`scx_task_has_kernel_slice`
- 外部函数：`scx_task_reenq_on_cap_revoke`
- 外部函数：`scx_task_sched`
- 外部函数：`scx_update_frozen_cpus`
- 外部函数：`scx_vexit`
- 外部函数：`set_cmask`
- 外部函数：`set_cpus_allowed_common`
- 外部函数：`set_task_cpu`
- 外部函数：`set_weight`
- 外部函数：`smp_mb`
- 外部函数：`sub_caps_updated`
- 外部函数：`task_allowed_on_cpu`
- 外部函数：`task_cpu`
- 外部函数：`task_current_donor`
- 外部函数：`task_has_idle_policy`
- 外部函数：`task_leave_custody`
- 外部函数：`task_on_cpu`
- 外部函数：`task_proxy_running_or_donating`
- 外部函数：`task_rq`
- 外部函数：`this_cpu_ptr`
- 外部函数：`unlikely`
- 外部函数：`unlink_dsq_and_switch_rq_lock`
- 外部函数：`update_curr`
- 外部函数：`update_curr_common`
- 外部函数：`update_locked_rq`
- 外部函数：`update_other_load_avgs`
- 外部函数：`update_rq_clock`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`ABBA`
- 大写宏：`BASE`
- 大写宏：`BPF`
- 大写宏：`BTF_ID_FLAGS`
- 大写宏：`BTF_KFUNCS_END`
- 大写宏：`BUG_ON`
- 大写宏：`CONFIG_SCHED_CORE`
- 大写宏：`CPU`
- 大写宏：`CPU0`
- 大写宏：`CPU1`
- 大写宏：`DEQUEUE_MOVE`
- 大写宏：`DEQUEUE_NOCLOCK`
- 大写宏：`DEQUEUE_SAVE`
- 大写宏：`DISPATCHING`
- 大写宏：`DSQ`
- 大写宏：`DSQ_LOCAL_ON`
- 大写宏：`EACCES`
- 大写宏：`EDEADLK`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`ENQUEUE_RESTORE`
- 大写宏：`ENQ_ACTION_DIRECT`
- 大写宏：`ENQ_ACTION_ENQUEUE`
- 大写宏：`ENQ_ACTION_LOCAL_NOREFILL`
- 大写宏：`ENQ_ACTION_NONE`
- 大写宏：`EXT`
- 大写宏：`FAIR`
- 大写宏：`FIFO`
- 大写宏：`IMMED`
- 大写宏：`INIT_DSQ_LIST_CURSOR`
- 大写宏：`KF_IMPLICIT_ARGS`
- 大写宏：`LIST_HEAD`
- 大写宏：`MAX_RT_PRIO`
- 大写宏：`NONE`
- 大写宏：`NULL`
- 大写宏：`PF_EXITING`
- 大写宏：`PREEMPT`
- 大写宏：`PRIQ`
- 大写宏：`QSEQ`
- 大写宏：`QUEUEING`
- 大写宏：`RB_CLEAR_NODE`
- 大写宏：`RB_EMPTY_NODE`
- 大写宏：`RB_EMPTY_ROOT`
- 大写宏：`READ_ONCE`
- 大写宏：`REJECT`
- 大写宏：`RESCUE`
- 大写宏：`RESTORE`
- 大写宏：`RMW`
- 大写宏：`RQCF_UPDATED`
- 大写宏：`SCX`
- 大写宏：`SCX_CALL_CID_OP_TASK`
- 大写宏：`SCX_CALL_OP_TASK`
- 大写宏：`SCX_CAP_BASE`
- 大写宏：`SCX_CAP_PERF`
- 大写宏：`SCX_CPUPERF_ONE`
- 大写宏：`SCX_DSP_NONE`
- 大写宏：`SCX_DSQ_BYPASS`
- 大写宏：`SCX_DSQ_FLAG_BUILTIN`
- 大写宏：`SCX_DSQ_GLOBAL`
- 大写宏：`SCX_DSQ_INVALID`
- 大写宏：`SCX_DSQ_ITER_REV`
- 大写宏：`SCX_DSQ_LNODE_ITER_CURSOR`
- 大写宏：`SCX_DSQ_LOCAL`
- 大写宏：`SCX_DSQ_LOCAL_CPU_MASK`
- 大写宏：`SCX_DSQ_LOCAL_ON`
- 大写宏：`SCX_DSQ_REJECT`
- 大写宏：`SCX_DSQ_RESCUE`
- 大写宏：`SCX_ENQ_APPLY_SLICE`
- 大写宏：`SCX_ENQ_BLOCKED`
- 大写宏：`SCX_ENQ_CLEAR_OPSS`
- 大写宏：`SCX_ENQ_DSQ_PRIQ`
- 大写宏：`SCX_ENQ_GDSQ_FALLBACK`
- 大写宏：`SCX_ENQ_HEAD`
- 大写宏：`SCX_ENQ_IGNORE_CAPS`
- 大写宏：`SCX_ENQ_NESTED`
- 大写宏：`SCX_ENQ_PREEMPT`
- 大写宏：`SCX_ENQ_PREEMPT_LAZY`
- 大写宏：`SCX_ENQ_REENQ`
- 大写宏：`SCX_ENQ_RESCUE`
- 大写宏：`SCX_ENQ_SLICE_DFL`
- 大写宏：`SCX_ENQ_WAKEUP`
- 大写宏：`SCX_EV_BYPASS_DISPATCH`
- 大写宏：`SCX_EV_DISPATCH_LOCAL_DSQ_OFFLINE`
- 大写宏：`SCX_EV_ENQ_SKIP_EXITING`
- 大写宏：`SCX_EV_ENQ_SKIP_MIGRATION_DISABLED`
- 大写宏：`SCX_EV_REENQ_IMMED`
- 大写宏：`SCX_EV_REENQ_REPEAT`
- 大写宏：`SCX_EV_REFILL_SLICE_DFL`
- 大写宏：`SCX_EV_SLICE_DENIED`
- 大写宏：`SCX_EV_SUB_CIDPERF_DENIED`
- 大写宏：`SCX_EXIT_DONE`
- 大写宏：`SCX_EXIT_ERROR_REENQ`
- 大写宏：`SCX_EXIT_NONE`
- 大写宏：`SCX_HAS_OP`
- 大写宏：`SCX_OPSS_DISPATCHING`
- 大写宏：`SCX_OPSS_NONE`
- 大写宏：`SCX_OPSS_QSEQ_MASK`
- 大写宏：`SCX_OPSS_QSEQ_SHIFT`
- 大写宏：`SCX_OPSS_QUEUED`
- 大写宏：`SCX_OPSS_QUEUEING`
- 大写宏：`SCX_OPSS_STATE_MASK`
- 大写宏：`SCX_OPS_ENQ_BLOCKED`
- 大写宏：`SCX_OPS_ENQ_EXITING`
- 大写宏：`SCX_OPS_ENQ_MIGRATION_DISABLED`
- 大写宏：`SCX_OPS_LAZY_RESCHED`
- 大写宏：`SCX_REENQ_ANY`
- 大写宏：`SCX_REENQ_CAP_REVOKE`
- 大写宏：`SCX_REENQ_MARKED`
- 大写宏：`SCX_REENQ_MAX_REPEAT`
- 大写宏：`SCX_REENQ_TSR_NOT_FIRST`
- 大写宏：`SCX_REENQ_TSR_RQ_OPEN`
- 大写宏：`SCX_RENQ_`
- 大写宏：`SCX_RQ_CAN_STOP_TICK`
- 大写宏：`SCX_RQ_IN_DISPATCH`
- 大写宏：`SCX_RQ_ONLINE`
- 大写宏：`SCX_RQ_PROXY_RETRY`
- 大写宏：`SCX_SLICE_INF`
- 大写宏：`SCX_SLICE_OOB_DUR_MASK`
- 大写宏：`SCX_SLICE_OOB_ID_MASK`
- 大写宏：`SCX_SLICE_OOB_ID_SHIFT`
- 大写宏：`SCX_TASK_DEAD`
- 大写宏：`SCX_TASK_DSQ_ON_PRIQ`
- 大写宏：`SCX_TASK_DSQ_REENQ`
- 大写宏：`SCX_TASK_ENABLED`
- 大写宏：`SCX_TASK_IMMED`
- 大写宏：`SCX_TASK_INIT`
- 大写宏：`SCX_TASK_INIT_BEGIN`
- 大写宏：`SCX_TASK_IN_CUSTODY`
- 大写宏：`SCX_TASK_ITER_BATCH`
- 大写宏：`SCX_TASK_NONE`
- 大写宏：`SCX_TASK_PROTECTED`
- 大写宏：`SCX_TASK_QUEUED`
- 大写宏：`SCX_TASK_READY`
- 大写宏：`SCX_TASK_REENQ_CAP`
- 大写宏：`SCX_TASK_REENQ_IMMED`
- 大写宏：`SCX_TASK_REENQ_KFUNC`
- 大写宏：`SCX_TASK_REENQ_PROXY`
- 大写宏：`SCX_TASK_REENQ_REASON_MASK`
- 大写宏：`SCX_TASK_RESET_RUNNABLE_AT`
- 大写宏：`SCX_TASK_STATE_MASK`
- 大写宏：`SINGLE_DEPTH_NESTING`
- 大写宏：`TASK_DEAD`
- 大写宏：`TASK_IMMED`
- 大写宏：`WARN_ONCE`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WEIGHT_IDLEPRIO`
- 大写宏：`WRITE_ONCE`
- 外部类型：`Affinity`
- 外部类型：`Always`
- 外部类型：`An`
- 外部类型：`As`
- 外部类型：`Before`
- 外部类型：`Built`
- 外部类型：`Bypass`
- 外部类型：`CPUs`
- 外部类型：`Can`
- 外部类型：`Cast`
- 外部类型：`Clear`
- 外部类型：`Clearing`
- 外部类型：`Commit`
- 外部类型：`Count`
- 外部类型：`DSQs`
- 外部类型：`Don`
- 外部类型：`Either`
- 外部类型：`Eject`
- 外部类型：`Find`
- 外部类型：`Fine`
- 外部类型：`For`
- 外部类型：`Global`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Invalid`
- 外部类型：`It`
- 外部类型：`Iterate`
- 外部类型：`Keep`
- 外部类型：`Local`
- 外部类型：`Move`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Optimize`
- 外部类型：`PRIQs`
- 外部类型：`Re`
- 外部类型：`Resolve`
- 外部类型：`See`
- 外部类型：`Send`
- 外部类型：`Set`
- 外部类型：`Skip`
- 外部类型：`Still`
- 外部类型：`Taking`
- 外部类型：`Task`
- 外部类型：`Test`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`They`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Trigger`
- 外部类型：`Unlike`
- 外部类型：`Unlinking`
- 外部类型：`Update`
- 外部类型：`User`
- 外部类型：`Verify`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`Whenever`
- 外部类型：`While`
- 外部类型：`Wrap`
- 外部类型：`Zero`
- 外部类型：`affinity_context`
- 外部类型：`bpf_prog_aux`
- 外部类型：`cpumask`
- 外部类型：`list_head`
- 外部类型：`rb_node`
- 外部类型：`rq`
- 外部类型：`rq_flags`
- 外部类型：`scx_cmask`
- 外部类型：`scx_cmask_ref`
- 外部类型：`scx_deferred_reenq_local`
- 外部类型：`scx_deferred_reenq_user`
- 外部类型：`scx_dispatch_q`
- 外部类型：`scx_dsq_list_node`
- 外部类型：`scx_dsq_pcpu`
- 外部类型：`scx_enable_args`
- 外部类型：`scx_exit_kind`
- 外部类型：`scx_sched`
- 外部类型：`scx_sched_pcpu`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-5e0188be23` → 本草稿移入 `cases/defect/auto-linux-5e0188be23/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
