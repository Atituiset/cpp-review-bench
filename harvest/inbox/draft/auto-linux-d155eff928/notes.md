# auto-linux-d155eff928

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#bf1079577a116f0685e7025b9ee2547345ee1c63](https://github.com/torvalds/linux/commit/bf1079577a116f0685e7025b9ee2547345ee1c63) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 102 |
| 编译错误数（gcc syntax-only） | 31（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bf1079577a116f0685e7025b9ee2547345ee1c63 (https://github.com/torvalds/linux/commit/bf1079577a116f0685e7025b9ee2547345ee1c63)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 7697；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bf1079577a116f0685e7025b9ee2547345ee1c63 Merge tag 'sched_ext-for-7.3-rc1-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/sched_ext :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -876,9 +876,9 @@ struct task_struct *scx_task_iter_next_locked(struct scx_task_iter *iter)
 		 * unloading. The init_tasks ("swappers") should be excluded
 		 * from the iteration because:
 		 *
-		 * - It's unsafe to use __setschduler_prio() on an init_task to
-		 *   determine the sched_class to use as it won't preserve its
-		 *   idle_sched_class.
+		 * - It's unsafe to use __setscheduler_class() on an init_task
+		 *   to determine the sched_class to use as it won't preserve
+		 *   its idle_sched_class.
 		 *
 		 * - ops.init/exit_task() can easily be confused if called with
 		 *   init_tasks as they, e.g., share PID 0.
@@ -2806,6 +2806,8 @@ static void dispatch_to_local_dsq(struct scx_sched *sch, struct rq *rq,
  * @p: task to finish dispatching
  * @qseq_at_dispatch: qseq when @p started getting dispatched
  * @dsq_id: destination DSQ ID
+ * @slice: slice carried by the insert verdict, 0 keeps the current value
+ * @vtime: vtime carried by the insert verdict, committed on PRIQ inserts
  * @enq_flags: %SCX_ENQ_*
  *
  * Dispatching to local DSQs may need to wait for queueing to complete or
@@ -5514,7 +5516,7 @@ static const struct kset_uevent_ops scx_uevent_ops = {
 };
 
 /*
- * Used by sched_fork() and __setscheduler_prio() to pick the matching
+ * Used by sched_fork() and __setscheduler_class() to pick the matching
  * sched_class. dl/rt are already handled.
  */
 bool task_should_scx(int policy)
@@ -7694,7 +7696,7 @@ static void scx_root_enable_workfn(struct kthread_work *work)
 	/*
 	 * Enable ops for every task. Fork is excluded by scx_fork_rwsem
 	 * preventing new tasks from being added. No need to exclude tasks
-	 * leaving as sched_ext_free() can handle both prepped and enabled
+	 * leaving as sched_ext_dead() can handle both prepped and enabled
 	 * tasks. Prep all tasks first and then enable them with preemption
 	 * disabled.
 	 *
@@ -7786,7 +7788,7 @@ static void scx_root_enable_workfn(struct kthread_work *work)
 
 	/*
 	 * We're fully committed and can't fail. The task READY -> ENABLED
-	 * transitions here are synchronized against sched_ext_free() through
+	 * transitions here are synchronized against sched_ext_dead() through
 	 * scx_tasks_lock.
 	 */
 	percpu_down_write(&scx_fork_rwsem);
@@ -8079,6 +8081,7 @@ static int bpf_scx_check_member(const struct btf_type *t,
 	case offsetof(struct sched_ext_ops, cgroup_init):
 	case offsetof(struct sched_ext_ops, cgroup_exit):
 	case offsetof(struct sched_ext_ops, cgroup_prep_move):
+	case offsetof(struct sched_ext_ops, cgroup_set_bandwidth):
 #endif
 	case offsetof(struct sched_ext_ops, cpu_online):
 	case offsetof(struct sched_ext_ops, cpu_offline):
@@ -9003,12 +9006,6 @@ static bool scx_dsq_move(struct bpf_iter_scx_dsq_kern *kit,
 	if (unlikely(READ_ONCE(sch->aborting)))
 		return false;
 
-	if (unlikely(!scx_task_on_sched(sch, p))) {
-		scx_error(sch, "scx_bpf_dsq_move[_vtime]() on %s[%d] but the task belongs to a different scheduler",
-			  p->comm, p->pid);
-		return false;
-	}
-
 	/*
 	 * Can be called from either ops.dispatch() holding the dispatched rq's
 	 * lock or any context where no rq lock is held. If latter, lock @p's
@@ -9040,6 +9037,17 @@ static bool scx_dsq_move(struct bpf_iter_scx_dsq_kern *kit,
 		goto out;
 	}
 
+	/*
+	 * @p has been on $src_dsq and can't move anymore. If @p is not on @sch,
+	 * the caller didn't have authority over @p at the time of the call.
+	 */
+	if (unlikely(!scx_task_on_sched(sch, p))) {
+		scx_error(sch, "scx_bpf_dsq_move[_vtime]() on %s[%d] but the task belongs to a different scheduler",
+			  p->comm, p->pid);
+		raw_spin_unlock(&src_dsq->lock);
+		goto out;
+	}
+
 	/* @p is still on $src_dsq and stable, determine the destination */
 	dst_dsq = find_dsq_for_dispatch(sch, locked_rq ?: this_rq(), dsq_id, task_cpu(p));
 
@@ -9765,7 +9773,7 @@ __bpf_kfunc struct task_struct *bpf_iter_scx_dsq_next(struct bpf_iter_scx_dsq *i
  * bpf_iter_scx_dsq_destroy - Destroy a DSQ iterator
  * @it: iterator to destroy
  *
- * U
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__attribute__`
- 外部函数：`__setschduler_prio`
- 外部函数：`__setscheduler_prio`
- 外部函数：`aligned`
- 外部函数：`atomic_read`
- 外部函数：`bpf_iter_scx_dsq_destroy`
- 外部函数：`bpf_scx_check_member`
- 外部函数：`bpf_scx_init_member`
- 外部函数：`cpu_possible`
- 外部函数：`cpu_rq`
- 外部函数：`cpu_to_node`
- 外部函数：`dispatch`
- 外部函数：`exit_task`
- 外部函数：`kthread_create`
- 外部函数：`likely`
- 外部函数：`next_active_class`
- 外部函数：`offsetof`
- 外部函数：`percpu_down_write`
- 外部函数：`rhashtable_lookup`
- 外部函数：`sched_ext_free`
- 外部函数：`scx_alloc_and_add_sched`
- 外部函数：`scx_cpu_ret`
- 外部函数：`scx_enable_state`
- 外部函数：`scx_enabled`
- 外部函数：`scx_error`
- 外部函数：`scx_task_on_sched`
- 外部函数：`task_cpu`
- 外部函数：`task_should_scx`
- 外部函数：`this_rq`
- 外部函数：`unlikely`
- 大写宏：`BUILD_BUG_ON`
- 大写宏：`CPU`
- 大写宏：`DSQ`
- 大写宏：`ENABLED`
- 大写宏：`PID`
- 大写宏：`READY`
- 大写宏：`READ_ONCE`
- 大写宏：`REJECT`
- 大写宏：`RESCUE`
- 大写宏：`SCHED_EXT`
- 大写宏：`SCHED_OTHER`
- 大写宏：`SCX_DISABLING`
- 大写宏：`SCX_DSQ_GLOBAL`
- 大写宏：`SCX_DSQ_LOCAL`
- 大写宏：`SCX_DSQ_LOCAL_CPU_MASK`
- 大写宏：`SCX_DSQ_LOCAL_ON`
- 大写宏：`SCX_ENQ_`
- 外部类型：`Both`
- 外部类型：`Built`
- 外部类型：`Can`
- 外部类型：`Catch`
- 外部类型：`DSQs`
- 外部类型：`Destroy`
- 外部类型：`Dispatching`
- 外部类型：`Enable`
- 外部类型：`Fork`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`Must`
- 外部类型：`No`
- 外部类型：`Prep`
- 外部类型：`The`
- 外部类型：`They`
- 外部类型：`This`
- 外部类型：`Undo`
- 外部类型：`Used`
- 外部类型：`We`
- 外部类型：`While`
- 外部类型：`rq`
- 外部类型：`sched_ext_ops`
- 外部类型：`sched_ext_ops_cid`
- 外部类型：`scx_dispatch_q`
- 外部类型：`scx_enable_state`
- 外部类型：`scx_sched`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-d155eff928` → 本草稿移入 `cases/defect/auto-linux-d155eff928/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
