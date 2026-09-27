// AUTO-DRAFT from torvalds/linux PR #efb27d47677397961c9017c0f8f469eb25a15d68
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <linux/execmem.h>
#include <linux/cleanup.h>
#include <linux/wait.h>

#include <asm/sections.h>
#include <asm/cacheflush.h>
/* …（同文件无关代码省略）… */
#define KPROBE_HASH_BITS 6
/* …（同文件无关代码省略）… */
static inline void set_kprobe_instance(struct kprobe *kp)
{
	__this_cpu_write(kprobe_instance, kp);
}
/* …（同文件无关代码省略）… */
static inline void reset_kprobe_instance(void)
{
	__this_cpu_write(kprobe_instance, NULL);
}
/* …（同文件无关代码省略）… */
struct kprobe *get_kprobe(void *addr)
{
	struct hlist_head *head;
	struct kprobe *p;

	head = &kprobe_table[hash_ptr(addr, KPROBE_HASH_BITS)];
	hlist_for_each_entry_rcu(p, head, hlist,
				 lockdep_is_held(&kprobe_mutex)) {
		if (p->addr == addr)
			return p;
	}

	return NULL;
}
/* …（同文件无关代码省略）… */
static inline bool kprobe_aggrprobe(struct kprobe *p)
{
	return p->pre_handler == aggr_pre_handler;
}
/* …（同文件无关代码省略）… */
static inline bool kprobe_unused(struct kprobe *p)
{
	return kprobe_aggrprobe(p) && kprobe_disabled(p) &&
	       list_empty(&p->list);
}
/* …（同文件无关代码省略）… */
static void free_aggr_kprobe(struct kprobe *p)
{
	struct optimized_kprobe *op;

	op = container_of(p, struct optimized_kprobe, kp);
	arch_remove_optimized_kprobe(op);
	arch_remove_kprobe(p);
	kfree(op);
}
/* …（同文件无关代码省略）… */
static inline int kprobe_optready(struct kprobe *p)
{
	struct optimized_kprobe *op;

	if (kprobe_aggrprobe(p)) {
		op = container_of(p, struct optimized_kprobe, kp);
		return arch_prepared_optinsn(&op->optinsn);
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static struct kprobe *get_optimized_kprobe(kprobe_opcode_t *addr)
{
	int i;
	struct kprobe *p = NULL;
	struct optimized_kprobe *op;

	/* Don't check i == 0, since that is a breakpoint case. */
	for (i = 1; !p && i < MAX_OPTIMIZED_LENGTH / sizeof(kprobe_opcode_t); i++)
		p = get_kprobe(addr - i);

	if (p && kprobe_optready(p)) {
		op = container_of(p, struct optimized_kprobe, kp);
		if (arch_within_optimized_kprobe(op, addr))
			return p;
	}

	return NULL;
}
/* …（同文件无关代码省略）… */
	OPTIMIZER_ST_FLUSHING = 2,
};

static DECLARE_COMPLETION(optimizer_completion);

#define OPTIMIZE_DELAY 5

/* …（同文件无关代码省略）… */
static void do_free_cleaned_kprobes(void)
{
	struct optimized_kprobe *op, *tmp;

	list_for_each_entry_safe(op, tmp, &freeing_list, list) {
		list_del_init(&op->list);
		if (WARN_ON_ONCE(!kprobe_unused(&op->kp))) {
			/*
			 * This must not happen, but if there is a kprobe
			 * still in use, keep it on kprobes hash list.
			 */
			continue;
		}

		/*
		 * The aggregator was holding back another probe while it sat on the
		 * unoptimizing/freeing lists.  Now that the aggregator has been fully
		 * reverted we can safely retry the optimization of that sibling.
		 */

		struct kprobe *_p = get_optimized_kprobe(op->kp.addr);
		if (unlikely(_p))
			optimize_kprobe(_p);

		free_aggr_kprobe(&op->kp);
	}
}
/* …（同文件无关代码省略）… */
		do_free_cleaned_kprobes();
	}

	/* Step 5: Kick optimizer again if needed. But if there is a flush requested, */
	if (completion_done(&optimizer_completion))
		complete(&optimizer_completion);

	if (!list_empty(&optimizing_list) || !list_empty(&unoptimizing_list))
		kick_kprobe_optimizer();	/*normal kick*/
/* …（同文件无关代码省略）… */
static void kick_kprobe_optimizer(void)
{
	lockdep_assert_held(&kprobe_mutex);
	if (atomic_cmpxchg(&optimizer_state,
		OPTIMIZER_ST_IDLE, OPTIMIZER_ST_KICKED) == OPTIMIZER_ST_IDLE)
		wake_up(&kprobe_optimizer_wait);
}
/* …（同文件无关代码省略）… */
	lockdep_assert_held(&kprobe_mutex);

	while (!list_empty(&optimizing_list) || !list_empty(&unoptimizing_list)) {
		init_completion(&optimizer_completion);
		/*
		 * Set state to OPTIMIZER_ST_FLUSHING and wake up the thread if it's
		 * idle. If it's already kicked, it will see the state change.
/* …（同文件无关代码省略）… */
			OPTIMIZER_ST_FLUSHING) != OPTIMIZER_ST_FLUSHING)
			wake_up(&kprobe_optimizer_wait);

		mutex_unlock(&kprobe_mutex);
		wait_for_completion(&optimizer_completion);
		mutex_lock(&kprobe_mutex);
	}
}

/* …（同文件无关代码省略）… */
#define optimize_kprobe(p)			do {} while (0)
/* …（同文件无关代码省略）… */
static int aggr_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct kprobe *kp;

	list_for_each_entry_rcu(kp, &p->list, list) {
		if (kp->pre_handler && likely(!kprobe_disabled(kp))) {
			set_kprobe_instance(kp);
			if (kp->pre_handler(kp, regs))
				return 1;
		}
		reset_kprobe_instance();
	}
	return 0;
}
