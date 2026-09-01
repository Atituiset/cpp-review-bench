// AUTO-DRAFT from torvalds/linux PR #bf1079577a116f0685e7025b9ee2547345ee1c63
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
static struct scx_dispatch_q *find_global_dsq(struct scx_sched *sch, s32 cpu)
{
	return &sch->pnode[cpu_to_node(cpu)]->global_dsq;
}
/* …（同文件无关代码省略）… */
static struct scx_dispatch_q *find_user_dsq(struct scx_sched *sch, u64 dsq_id)
{
	return rhashtable_lookup(&sch->dsq_hash, &dsq_id, dsq_hash_params);
}
/* …（同文件无关代码省略）… */
struct bpf_iter_scx_dsq {
	u64				__opaque[6];
} __attribute__((aligned(8)));
/* …（同文件无关代码省略）… */
		 * unloading. The init_tasks ("swappers") should be excluded
		 * from the iteration because:
		 *
		 * - It's unsafe to use __setschduler_prio() on an init_task to
		 *   determine the sched_class to use as it won't preserve its
		 *   idle_sched_class.
		 *
		 * - ops.init/exit_task() can easily be confused if called with
		 *   init_tasks as they, e.g., share PID 0.
/* …（同文件无关代码省略）… */
static enum scx_enable_state scx_enable_state(void)
{
	return atomic_read(&scx_enable_state_var);
}
/* …（同文件无关代码省略）… */
static inline bool __cpu_valid(s32 cpu)
{
	return likely(cpu >= 0 && cpu < nr_cpu_ids && cpu_possible(cpu));
}
/* …（同文件无关代码省略）… */
bool scx_cpu_valid(struct scx_sched *sch, s32 cpu, const char *where)
{
	if (__cpu_valid(cpu)) {
		return true;
	} else {
		scx_error(sch, "invalid CPU %d%s%s", cpu, where ? " " : "", where ?: "");
		return false;
	}
}
/* …（同文件无关代码省略）… */
static struct scx_dispatch_q *find_dsq_for_dispatch(struct scx_sched *sch,
						    struct rq *rq, u64 dsq_id,
						    s32 tcpu)
{
	struct scx_dispatch_q *dsq;

	if (dsq_id == SCX_DSQ_LOCAL)
		return &rq->scx.local_dsq;

	if ((dsq_id & SCX_DSQ_LOCAL_ON) == SCX_DSQ_LOCAL_ON) {
		s32 cpu = scx_cpu_ret(sch, dsq_id & SCX_DSQ_LOCAL_CPU_MASK);

		if (!scx_cpu_valid(sch, cpu, "in SCX_DSQ_LOCAL_ON dispatch verdict"))
			return find_global_dsq(sch, tcpu);

		return &cpu_rq(cpu)->scx.local_dsq;
	}

	if (dsq_id == SCX_DSQ_GLOBAL)
		dsq = find_global_dsq(sch, tcpu);
	else
		dsq = find_user_dsq(sch, dsq_id);

	/*
	 * Built-in DSQs are never inserted into dsq_hash, so REJECT and RESCUE
	 * hit the error below. They cannot be reached with an ID.
	 */
	if (unlikely(!dsq)) {
		scx_error(sch, "non-existent DSQ 0x%llx", dsq_id);
		return find_global_dsq(sch, tcpu);
	}

	return dsq;
}
/* …（同文件无关代码省略）… */
 * @p: task to finish dispatching
 * @qseq_at_dispatch: qseq when @p started getting dispatched
 * @dsq_id: destination DSQ ID
 * @enq_flags: %SCX_ENQ_*
 *
 * Dispatching to local DSQs may need to wait for queueing to complete or
/* …（同文件无关代码省略）… */
};

/*
 * Used by sched_fork() and __setscheduler_prio() to pick the matching
 * sched_class. dl/rt are already handled.
 */
bool task_should_scx(int policy)
{
	/* if disabled, nothing should be on it */
	if (!scx_enabled())
		return false;

	/* scx is taking over all SCHED_OTHER and SCHED_EXT tasks */
	if (READ_ONCE(scx_switching_all))
		return true;

	/*
	 * scx is tearing down - keep new SCHED_EXT tasks out.
	 *
	 * Must come after scx_switching_all test, which serves as a proxy
	 * for __scx_switched_all. While __scx_switched_all is set, we must
	 * return true via the branch above: a fork routed to fair would
	 * stall because next_active_class() skips fair.
	 *
	 * This can develop into a deadlock - scx holds scx_enable_mutex across
	 * kthread_create() in scx_alloc_and_add_sched(); if the new kthread is
	 * the stalled task, the disable path can never grab the mutex to clear
	 * scx_switching_all.
	 */
	if (unlikely(scx_enable_state() == SCX_DISABLING))
		return false;

	return policy == SCHED_EXT;
}
/* …（同文件无关代码省略）… */
	/*
	 * Enable ops for every task. Fork is excluded by scx_fork_rwsem
	 * preventing new tasks from being added. No need to exclude tasks
	 * leaving as sched_ext_free() can handle both prepped and enabled
	 * tasks. Prep all tasks first and then enable them with preemption
	 * disabled.
	 *
/* …（同文件无关代码省略）… */

	/*
	 * We're fully committed and can't fail. The task READY -> ENABLED
	 * transitions here are synchronized against sched_ext_free() through
	 * scx_tasks_lock.
	 */
	percpu_down_write(&scx_fork_rwsem);
/* …（同文件无关代码省略）… */
	case offsetof(struct sched_ext_ops, cgroup_init):
	case offsetof(struct sched_ext_ops, cgroup_exit):
	case offsetof(struct sched_ext_ops, cgroup_prep_move):
#endif
	case offsetof(struct sched_ext_ops, cpu_online):
	case offsetof(struct sched_ext_ops, cpu_offline):
/* …（同文件无关代码省略）… */
	if (unlikely(READ_ONCE(sch->aborting)))
		return false;

	if (unlikely(!scx_task_on_sched(sch, p))) {
		scx_error(sch, "scx_bpf_dsq_move[_vtime]() on %s[%d] but the task belongs to a different scheduler",
			  p->comm, p->pid);
		return false;
	}

	/*
	 * Can be called from either ops.dispatch() holding the dispatched rq's
	 * lock or any context where no rq lock is held. If latter, lock @p's
/* …（同文件无关代码省略）… */
		goto out;
	}

	/* @p is still on $src_dsq and stable, determine the destination */
	dst_dsq = find_dsq_for_dispatch(sch, locked_rq ?: this_rq(), dsq_id, task_cpu(p));

/* …（同文件无关代码省略）… */
 * bpf_iter_scx_dsq_destroy - Destroy a DSQ iterator
 * @it: iterator to destroy
 *
 * Undo scx_iter_scx_dsq_new().
 */
__bpf_kfunc void bpf_iter_scx_dsq_destroy(struct bpf_iter_scx_dsq *it)
{
/* …（同文件无关代码省略）… */
static int __init scx_init(void)
{
	int ret;

	/*
	 * sched_ext_ops_cid mirrors sched_ext_ops up to and including @priv.
	 * Both bpf_scx_init_member() and bpf_scx_check_member() use offsets
	 * from struct sched_ext_ops; sched_ext_ops_cid relies on those offsets
	 * matching for the shared fields. Catch any drift at boot.
	 */
#define CID_OFFSET_MATCH(cpu_field, cid_field)					\
	BUILD_BUG_ON(offsetof(struct sched_ext_ops, cpu_field) !=		\
		     offsetof(struct sched_ext_ops_cid, cid_field))
	/* data fields used by bpf_scx_init_member() */
	CID_OFFSET_MATCH(dispatch_max_batch, dispatch_max_batch);
	CID_OFFSET_MATCH(flags, flags);
	CID_OFFSET_MATCH(name, name);
	CID_OFFSET_MATCH(timeout_ms, timeout_ms);
	CID_OFFSET_MATCH(exit_dump_
