# auto-linux-3fc2ea76b6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#762122d75e50e4919ef77afc2dffdc4931c5be87](https://github.com/torvalds/linux/commit/762122d75e50e4919ef77afc2dffdc4931c5be87) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 245 |
| 编译错误数（gcc syntax-only） | 135（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #762122d75e50e4919ef77afc2dffdc4931c5be87 (https://github.com/torvalds/linux/commit/762122d75e50e4919ef77afc2dffdc4931c5be87)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2194；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 762122d75e50e4919ef77afc2dffdc4931c5be87 Merge tag 'sched_ext-for-7.3-rc6-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/sched_ext :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2057,8 +2057,14 @@ void scx_do_enqueue_task(struct rq *rq, struct task_struct *p, u64 enq_flags,
 	if (unlikely(!SCX_HAS_OP(sch, enqueue)))
 		goto global;
 
-	/* DSQ bypass didn't trigger, enqueue on the BPF scheduler */
-	qseq = rq->scx.ops_qseq++ << SCX_OPSS_QSEQ_SHIFT;
+	/*
+	 * DSQ bypass didn't trigger, enqueue on the BPF scheduler. Wrap the
+	 * per-task qseq counter where the QSEQ field wraps and skip 0, which is
+	 * what scx_bpf_dsq_insert() records for a task in NONE or DISPATCHING.
+	 */
+	p->scx.ops_qseq = ((p->scx.ops_qseq + 1) &
+			   (SCX_OPSS_QSEQ_MASK >> SCX_OPSS_QSEQ_SHIFT)) ?: 1;
+	qseq = (unsigned long)p->scx.ops_qseq << SCX_OPSS_QSEQ_SHIFT;
 
 	WARN_ON_ONCE(atomic_long_read(&p->scx.ops_state) != SCX_OPSS_NONE);
 	atomic_long_set(&p->scx.ops_state, SCX_OPSS_QUEUEING | qseq);
@@ -2123,8 +2129,8 @@ static void set_task_runnable(struct rq *rq, struct task_struct *p)
 	}
 
 	/*
-	 * list_add_tail() must be used. scx_bypass() depends on tasks being
-	 * appended to the runnable_list.
+	 * list_add_tail() must be used. scx_bypass() and rq_offline_scx()
+	 * depend on tasks being appended to the runnable_list.
 	 */
 	list_add_tail(&p->scx.runnable_node, &rq->scx.runnable_list);
 
@@ -2152,6 +2158,13 @@ static void enqueue_task_scx(struct rq *rq, struct task_struct *p, int core_enq_
 	int sticky_cpu = p->scx.sticky_cpu;
 	u64 enq_flags = core_enq_flags | rq->scx.remote_activate_enq_flags;
 
+	/*
+	 * An SCX-internal migration ends on arrival. Clear sticky_cpu so @p can
+	 * leave custody when inserted into the destination DSQ.
+	 */
+	if (sticky_cpu >= 0)
+		p->scx.sticky_cpu = -1;
+
 	/*
 	 * SCX_RQ_IN_WAKEUP promises a task_woken_scx() call once this enqueue
 	 * returns. Only the core's wakeup path delivers one. The flags stashed
@@ -2190,9 +2203,6 @@ static void enqueue_task_scx(struct rq *rq, struct task_struct *p, int core_enq_
 		dl_server_start(&rq->ext_server);
 
 	scx_do_enqueue_task(rq, p, enq_flags, sticky_cpu);
-
-	if (sticky_cpu >= 0)
-		p->scx.sticky_cpu = -1;
 out:
 	rq->scx.flags &= ~SCX_RQ_IN_WAKEUP;
 
@@ -3731,8 +3741,26 @@ static void rq_online_scx(struct rq *rq)
 
 static void rq_offline_scx(struct rq *rq)
 {
+	struct task_struct *p, *n;
+
 	rq->scx.flags &= ~SCX_RQ_ONLINE;
+
+	/* sched domain rebuilds call rq_offline with the CPU staying alive */
+	if (cpu_active(cpu_of(rq)))
+		return;
+
 	scx_rescue_flush(rq);
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
 }
 
 static bool check_rq_for_timeouts(struct rq *rq)
@@ -6947,9 +6975,9 @@ static void scx_dump_cpu(struct scx_sched *sch, struct seq_buf *s,
 	seq_buf_init(&ns, buf, avail);
 
 	dump_newline(&ns);
-	scx_dump_line(&ns, "CPU %-4d: nr_run=%u flags=0x%x cpu_rel=%d ops_qseq=%lu ksync=%lu",
+	scx_dump_line(&ns, "CPU %-4d: nr_run=%u flags=0x%x cpu_rel=%d ksync=%lu",
 		      cpu, rq->scx.nr_running, rq->scx.flags, rq->scx.cpu_released,
-		      rq->scx.ops_qseq, rq->scx.kick_sync);
+		      rq->scx.kick_sync);
 	scx_rescue_dump(&ns, rq);
 	scx_dump_line(&ns, "          curr=%s[%d] class=%ps",
 		      rq->curr->comm, rq->curr->pid, rq->curr->sched_class);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__scx_add_event`
- 外部函数：`__scx_exit`
- 外部函数：`_locked`
- 外部函数：`atomic64_read`
- 外部函数：`atomic64_set`
- 外部函数：`atomic_long_read`
- 外部函数：`atomic_long_set`
- 外部函数：`atomic_long_set_release`
- 外部函数：`atomic_try_cmpxchg`
- 外部函数：`container_of`
- 外部函数：`cpu_active`
- 外部函数：`cpu_of`
- 外部函数：`cpu_possible`
- 外部函数：`cpu_rq`
- 外部函数：`cpu_to_node`
- 外部函数：`deactivate_task`
- 外部函数：`dec_bypass_depth`
- 外部函数：`dequeue_task_scx`
- 外部函数：`dispatch`
- 外部函数：`dispatch_one`
- 外部函数：`dl_server_start`
- 外部函数：`enqueue`
- 外部函数：`finish_dispatch`
- 外部函数：`guard`
- 外部函数：`in_nmi`
- 外部函数：`inc_bypass_depth`
- 外部函数：`irq_work_queue`
- 外部函数：`irq_work_queue_on`
- 外部函数：`is_migration_disabled`
- 外部函数：`jiffies_to_msecs`
- 外部函数：`likely`
- 外部函数：`list_add`
- 外部函数：`list_add_tail`
- 外部函数：`list_empty`
- 外部函数：`lock`
- 外部函数：`lockdep_assert_held`
- 外部函数：`lockdep_assert_preemption_disabled`
- 外部函数：`lockdep_assert_rq_held`
- 外部函数：`move_remote_task_to_local_dsq`
- 外部函数：`ops_dequeue`
- 外部函数：`paths`
- 外部函数：`raw_spin_lock`
- 外部函数：`raw_spin_lock_nested`
- 外部函数：`raw_spin_unlock`
- 外部函数：`rb_add`
- 外部函数：`rb_prev`
- 外部函数：`rcu_assign_pointer`
- 外部函数：`rcu_dereference_bh`
- 外部函数：`resched_curr`
- 外部函数：`rhashtable_lookup`
- 外部函数：`rq_lock_irqsave`
- 外部函数：`rq_unlock_irqrestore`
- 外部函数：`sched_class_above`
- 外部函数：`scoped_guard`
- 外部函数：`scx_bpf_dsq_nr_queued`
- 外部函数：`scx_bypass`
- 外部函数：`scx_bypass_dsq`
- 外部函数：`scx_bypassing`
- 外部函数：`scx_cpu_ret`
- 外部函数：`scx_disable_workfn`
- 外部函数：`scx_dump_line`
- 外部函数：`scx_dump_state`
- 外部函数：`scx_error`
- 外部函数：`scx_link_sched`
- 外部函数：`scx_parent`
- 外部函数：`scx_rescue_dump`
- 外部函数：`scx_rescue_flush`
- 外部函数：`scx_resolve_local_dsq`
- 外部函数：`scx_schedule_reenq_local`
- 外部函数：`scx_task_sched`
- 外部函数：`scx_wakeup_preempt`
- 外部函数：`seq_buf_init`
- 外部函数：`seq_buf_putc`
- 外部函数：`seq_buf_vprintf`
- 外部函数：`stack_trace_save`
- 外部函数：`task_cpu`
- 外部函数：`task_rq`
- 外部函数：`task_woken_scx`
- 外部函数：`tasks`
- 外部函数：`this_cpu_ptr`
- 外部函数：`time_after`
- 外部函数：`time_before64`
- 外部函数：`trace_call__sched_ext_dump`
- 外部函数：`trace_sched_ext_dump`
- 外部函数：`trace_sched_ext_dump_enabled`
- 外部函数：`trace_sched_ext_exit`
- 外部函数：`unlikely`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vscnprintf`
- 外部函数：`wakeup_preempt`
- 大写宏：`BPF`
- 大写宏：`CONFIG_EXT_SUB_SCHED`
- 大写宏：`CONFIG_STACKTRACE`
- 大写宏：`CONFIG_TRACEPOINTS`
- 大写宏：`CPU`
- 大写宏：`DISPATCH`
- 大写宏：`DISPATCHING`
- 大写宏：`DSQ`
- 大写宏：`DSQ_LOCAL_ON`
- 大写宏：`FIFO`
- 大写宏：`IMMED`
- 大写宏：`IPI`
- 大写宏：`IRQ`
- 大写宏：`NMI`
- 大写宏：`NULL`
- 大写宏：`PF_EXITING`
- 大写宏：`PREEMPT`
- 大写宏：`PRIQ`
- 大写宏：`QUEUEING`
- 大写宏：`RB_EMPTY_NODE`
- 大写宏：`RB_EMPTY_ROOT`
- 大写宏：`READ_ONCE`
- 大写宏：`REJECT`
- 大写宏：`RESCUE`
- 大写宏：`RESTORE`
- 大写宏：`RMW`
- 大写宏：`SAVE`
- 大写宏：`SCX`
- 大写宏：`SCX_CALL_OP_TASK`
- 大写宏：`SCX_DSQ_BYPASS`
- 大写宏：`SCX_DSQ_FLAG_BUILTIN`
- 大写宏：`SCX_DSQ_GLOBAL`
- 大写宏：`SCX_DSQ_INVALID`
- 大写宏：`SCX_DSQ_LNODE_ITER_CURSOR`
- 大写宏：`SCX_DSQ_LOCAL`
- 大写宏：`SCX_DSQ_LOCAL_CPU_MASK`
- 大写宏：`SCX_DSQ_LOCAL_ON`
- 大写宏：`SCX_DSQ_REJECT`
- 大写宏：`SCX_DSQ_RESCUE`
- 大写宏：`SCX_ENQ_APPLY_SLICE`
- 大写宏：`SCX_ENQ_CLEAR_OPSS`
- 大写宏：`SCX_ENQ_DSQ_PRIQ`
- 大写宏：`SCX_ENQ_GDSQ_FALLBACK`
- 大写宏：`SCX_ENQ_HEAD`
- 大写宏：`SCX_ENQ_IMMED`
- 大写宏：`SCX_ENQ_NESTED`
- 大写宏：`SCX_ENQ_PREEMPT`
- 大写宏：`SCX_ENQ_REENQ`
- 大写宏：`SCX_ENQ_RESCUE`
- 大写宏：`SCX_ENQ_SLICE_DFL`
- 大写宏：`SCX_EV_BYPASS_DISPATCH`
- 大写宏：`SCX_EV_ENQ_SKIP_EXITING`
- 大写宏：`SCX_EV_ENQ_SKIP_MIGRATION_DISABLED`
- 大写宏：`SCX_EV_REENQ_REPEAT`
- 大写宏：`SCX_EV_REFILL_SLICE_DFL`
- 大写宏：`SCX_EV_SLICE_DENIED`
- 大写宏：`SCX_EXIT_BT_LEN`
- 大写宏：`SCX_EXIT_DONE`
- 大写宏：`SCX_EXIT_ERROR`
- 大写宏：`SCX_EXIT_ERROR_BPF`
- 大写宏：`SCX_EXIT_ERROR_REENQ`
- 大写宏：`SCX_EXIT_ERROR_RESCUE`
- 大写宏：`SCX_EXIT_ERROR_STALL`
- 大写宏：`SCX_EXIT_MSG_LEN`
- 大写宏：`SCX_EXIT_NONE`
- 大写宏：`SCX_EXIT_PARENT`
- 大写宏：`SCX_EXIT_PARENT_KILL`
- 大写宏：`SCX_EXIT_SYSRQ`
- 大写宏：`SCX_EXIT_UNREG`
- 大写宏：`SCX_EXIT_UNREG_BPF`
- 大写宏：`SCX_EXIT_UNREG_KERN`
- 大写宏：`SCX_HAS_OP`
- 大写宏：`SCX_OPSS_DISPATCHING`
- 大写宏：`SCX_OPSS_NONE`
- 大写宏：`SCX_OPSS_QSEQ_SHIFT`
- 大写宏：`SCX_OPSS_QUEUED`
- 大写宏：`SCX_OPSS_QUEUEING`
- 大写宏：`SCX_OPSS_STATE_MASK`
- 大写宏：`SCX_OPS_ENQ_EXITING`
- 大写宏：`SCX_OPS_ENQ_MIGRATION_DISABLED`
- 大写宏：`SCX_REENQ_MAX_REPEAT`
- 大写宏：`SCX_RQ_BAL_CB_PENDING`
- 大写宏：`SCX_RQ_IN_DISPATCH`
- 大写宏：`SCX_RQ_IN_WAKEUP`
- 大写宏：`SCX_RQ_ONLINE`
- 大写宏：`SCX_TASK_DSQ_ON_PRIQ`
- 大写宏：`SCX_TASK_IMMED`
- 大写宏：`SCX_TASK_IN_CUSTODY`
- 大写宏：`SCX_TASK_PROTECTED`
- 大写宏：`SCX_TASK_QUEUED`
- 大写宏：`SINGLE_DEPTH_NESTING`
- 大写宏：`TASK_IMMED`
- 大写宏：`UNKNOWN`
- 大写宏：`WAKEUP`
- 大写宏：`WARN`
- 大写宏：`WARN_ONCE`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WRITE_ONCE`
- 外部类型：`An`
- 外部类型：`As`
- 外部类型：`Blame`
- 外部类型：`Both`
- 外部类型：`Built`
- 外部类型：`CPUs`
- 外部类型：`Clear`
- 外部类型：`Commit`
- 外部类型：`Currently`
- 外部类型：`DSQs`
- 外部类型：`Don`
- 外部类型：`Eject`
- 外部类型：`Example`
- 外部类型：`Find`
- 外部类型：`First`
- 外部类型：`For`
- 外部类型：`Global`
- 外部类型：`Here`
- 外部类型：`IRQs`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`It`
- 外部类型：`Just`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`Once`
- 外部类型：`Only`
- 外部类型：`PRIQs`
- 外部类型：`Queue`
- 外部类型：`Re`
- 外部类型：`Rejects`
- 外部类型：`Schedule`
- 外部类型：`Second`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`Skip`
- 外部类型：`Still`
- 外部类型：`Sweep`
- 外部类型：`Task`
- 外部类型：`Test`
- 外部类型：`The`
- 外部类型：`They`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Update`
- 外部类型：`Use`
- 外部类型：`We`
- 外部类型：`Whenever`
- 外部类型：`list_head`
- 外部类型：`rb_node`
- 外部类型：`rq`
- 外部类型：`rq_flags`
- 外部类型：`scx_dispatch_q`
- 外部类型：`scx_dsq_list_node`
- 外部类型：`scx_exit_info`
- 外部类型：`scx_exit_kind`
- 外部类型：`scx_sched`
- 外部类型：`seq_buf`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-3fc2ea76b6` → 本草稿移入 `cases/defect/auto-linux-3fc2ea76b6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
