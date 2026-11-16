// AUTO-DRAFT from torvalds/linux PR #a9daf327f3ff90625204876981f88cfe2b6f6fff
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	SCX_DSP_NONE,		/* nothing to run */
	SCX_DSP_LOCAL,		/* local DSQ has tasks */
	SCX_DSP_PREV,		/* keep running @prev */
	SCX_DSP_RETRY,		/* pick helpers only: restart the pick */
};

/*
 * One user of this function is scx_bpf_sub_dispatch() which can be called
 * recursively as sub-sched dispatches nest. Always inline to reduce stack usage
/* …（同文件无关代码省略）… */
	struct scx_dsp_ctx *dspc = &this_cpu_ptr(sch->pcpu)->dsp_ctx;
	int nr_loops = SCX_DSP_MAX_LOOPS;
	s32 cpu = cpu_of(rq);
	bool prev_on_sch = (prev->sched_class == &ext_sched_class) &&
		scx_task_on_sched(sch, prev);

	if (scx_consume_global_dsq(sch, rq))
		return SCX_DSP_LOCAL;
/* …（同文件无关代码省略）… */
	/*
	 * scx_rq_online() can't be used. Its cpu_active() test goes false
	 * before CPU hotplug waits for an RCU grace period, and
	 * rq_offline_scx() moves this CPU's tasks to the local DSQ only after
	 * the wait. The grace period can depend on those tasks running.
	 */
	if (unlikely(!SCX_HAS_OP(sch, dispatch)) || !(rq->scx.flags & SCX_RQ_ONLINE))
/* …（同文件无关代码省略）… */
			rq->scx.sub_dispatch_prev = prev;
#endif

		SCX_CALL_OP(sch, dispatch, rq, scx_cpu_arg(cpu),
			    prev_on_sch ? prev : NULL);

/* …（同文件无关代码省略）… */

		scx_flush_dispatch_buf(sch, rq);

		if ((prev->scx.flags & SCX_TASK_QUEUED) && prev->scx.slice)
			return SCX_DSP_PREV;
		if (rq->scx.local_dsq.nr)
			return SCX_DSP_LOCAL;
