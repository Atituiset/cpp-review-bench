// AUTO-DRAFT from torvalds/linux PR #e4fb7e5e8e7e9edc167c3057802e440b77dcb856
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define KPROBE_HASH_BITS 6
/* …（同文件无关代码省略）… */
kprobe_opcode_t * __weak kprobe_lookup_name(const char *name,
					unsigned int __unused)
{
	return ((kprobe_opcode_t *)(kallsyms_lookup_name(name)));
}
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
	list_for_each_entry_safe(op, tmp, free_list, list) {
		/* Switching from detour code to origin */
		op->kp.flags &= ~KPROBE_FLAG_OPTIMIZED;
		/* Disarm probes if marked disabled and not gone */
		if (kprobe_disabled(&op->kp) && !kprobe_gone(&op->kp))
			arch_disarm_kprobe(&op->kp);
		if (kprobe_unused(&op->kp)) {
			/*
/* …（同文件无关代码省略）… */
		list_for_each_entry(_op, &optprobe_gens[i].unoptimizing_list, list) {
			if (op == _op)
				return true;
		}
/* …（同文件无关代码省略）… */
static void force_unoptimize_kprobe(struct optimized_kprobe *op)
{
	lockdep_assert_cpus_held();
	arch_unoptimize_kprobe(op);
	op->kp.flags &= ~KPROBE_FLAG_OPTIMIZED;
}
/* …（同文件无关代码省略）… */
				 * for cooling down (or freeing if unused).
				 */
				force_unoptimize_kprobe(op);
				if (kprobe_unused(&op->kp)) {
					if (!kprobe_gone(&op->kp))
						arch_disarm_kprobe(&op->kp);
					hlist_del_init_rcu(&op->kp.hlist);
					list_move(&op->list, &cur->freeing_list);
				} else {
/* …（同文件无关代码省略）… */
	 */
	op = container_of(ap, struct optimized_kprobe, kp);
	WARN_ON_ONCE(list_empty(&op->list));
	/* Enable the probe again */
	ap->flags &= ~KPROBE_FLAG_DISABLED;
	/* Optimize it again. (remove from 'op->list') */
/* …（同文件无关代码省略）… */
#define optimize_kprobe(p)			do {} while (0)
#define unoptimize_kprobe(p, f)			do {} while (0)
/* …（同文件无关代码省略）… */
#define __disarm_kprobe(p, o)			arch_disarm_kprobe(p)
#define kprobe_disarmed(p)			kprobe_disabled(p)
/* …（同文件无关代码省略）… */
static int __disarm_kprobe_ftrace(struct kprobe *p, struct ftrace_ops *ops,
				  int *cnt)
{
	int ret;

	lockdep_assert_held(&kprobe_mutex);
	if (unlikely(kprobe_ftrace_disabled)) {
		/* Now ftrace is disabled forever, disarm is already done. */
		return 0;
	}

	if (*cnt == 1) {
		ret = unregister_ftrace_function(ops);
		if (WARN(ret < 0, "Failed to unregister kprobe-ftrace (error %d)\n", ret))
			return ret;
	}

	(*cnt)--;

	ret = ftrace_set_filter_ip(ops, (unsigned long)p->addr, 1, 0);
	WARN_ONCE(ret < 0, "Failed to disarm kprobe-ftrace at %pS (error %d)\n",
		  p->addr, ret);
	return ret;
}
/* …（同文件无关代码省略）… */
static int disarm_kprobe_ftrace(struct kprobe *p)
{
	bool ipmodify = (p->post_handler != NULL);

	return __disarm_kprobe_ftrace(p,
		ipmodify ? &kprobe_ipmodify_ops : &kprobe_ftrace_ops,
		ipmodify ? &kprobe_ipmodify_enabled : &kprobe_ftrace_enabled);
}
/* …（同文件无关代码省略）… */
static int disarm_kprobe(struct kprobe *kp, bool reopt)
{
	if (unlikely(kprobe_ftrace(kp)))
		return disarm_kprobe_ftrace(kp);

	guard(cpus_read_lock)();
	guard(mutex)(&text_mutex);
	__disarm_kprobe(kp, reopt);
	return 0;
}
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
/* …（同文件无关代码省略）… */
/* Add the new probe to 'ap->list'. */
static int add_new_kprobe(struct kprobe *ap, struct kprobe *p)
{
	if (p->post_handler)
		unoptimize_kprobe(ap, true);	/* Fall back to normal kprobe */

	list_add_rcu(&p->list, &ap->list);
/* …（同文件无关代码省略）… */
kprobe_opcode_t *__weak arch_adjust_kprobe_addr(unsigned long addr,
						unsigned long offset,
						bool *on_func_entry)
{
	*on_func_entry = !offset;
	return (kprobe_opcode_t *)(addr + offset);
}
/* …（同文件无关代码省略）… */
static kprobe_opcode_t *
_kprobe_addr(kprobe_opcode_t *addr, const char *symbol_name,
	     unsigned long offset, bool *on_func_entry)
{
	if ((symbol_name && addr) || (!symbol_name && !addr))
		return ERR_PTR(-EINVAL);

	if (symbol_name) {
		/*
		 * Input: @sym + @offset
		 * Output: @addr + @offset
		 *
		 * NOTE: kprobe_lookup_name() does *NOT* fold the offset
		 *       argument into it's output!
		 */
		addr = kprobe_lookup_name(symbol_name, offset);
		if (!addr)
			return ERR_PTR(-ENOENT);
	}

	/*
	 * So here we have @addr + @offset, displace it into a new
	 * @addr' + @offset' where @addr' is the symbol start address.
	 */
	addr = (void *)addr + offset;
	if (!kallsyms_lookup_size_offset((unsigned long)addr, NULL, &offset))
		return ERR_PTR(-ENOENT);
	addr = (void *)addr - offset;

	/*
	 * Then ask the architecture to re-combine them, taking care of
	 * magical function entry details while telling us if this was indeed
	 * at the start of the function.
	 */
	addr = arch_adjust_kprobe_addr((unsigned long)addr, offset, on_func_entry);
	if (!addr)
		return ERR_PTR(-EINVAL);

	return addr;
}

static kprobe_opcode_t *kprobe_addr(struct kprobe *p)
{
	bool on_func_entry;

	return _kprobe_addr(p->addr, p->symbol_name, p->offset, &on_func_entry);
}

/*
 * Check the 'p' is valid and return the aggregator kprobe
 * at the same address.
 */
static struct kprobe *__get_valid_kprobe(struct kprobe *p)
{
	struct kprobe *ap, *list_p;

	lockdep_assert_held(&kprobe_mutex);

	ap = get_kprobe(p->addr);
	if (unlikely(!ap))
		return NULL;

	if (p == ap)
		return ap;

	list_for_each_entry(list_p, &ap->list, list)
		if (list_p == p)
		/* kprobe p is a valid probe */
			return ap;

	return NULL;
}

/*
 * Warn and return error if the kprobe is being re-registered since
 * there must be a software bug.
 */
static inline int warn_kprobe_rereg(struct kprobe *p)
{
	guard(mutex)(&kprobe_mutex);

	if (WARN_ON_ONCE(__get_valid_kprobe(p)))
		return -EINVAL;

	return 0;
}

static int check_ftrace_location(struct kprobe *p)
{
	unsigned long addr = (unsigned long)p->addr;
/* …（同文件无关代码省略）… */
		if (ret) {
			hlist_del_rcu(&p->hlist);
			synchronize_rcu();
		}
	}

/* …（同文件无关代码省略）… */
		return PTR_ERR(addr);
	p->addr = addr;

	ret = warn_kprobe_rereg(p);
	if (ret)
		return ret;

/* …（同文件无关代码省略）… */
static bool aggr_kprobe_disabled(struct kprobe *ap)
{
	struct kprobe *kp;

	lockdep_assert_held(&kprobe_mutex);

	list_for_each_entry(kp, &ap->list, list)
		if (!kprobe_disabled(kp))
			/*
			 * Since there is an active probe on the list,
			 * we can't disable this 'ap'.
			 */
			return false;

	return true;
}
/* …（同文件无关代码省略）… */
static struct kprobe *__disable_kprobe(struct kprobe *p)
{
	struct kprobe *orig_p;
	int ret;

	lockdep_assert_held(&kprobe_mutex);

	/* Get an original kprobe for return */
	orig_p = __get_valid_kprobe(p);
	if (unlikely(orig_p == NULL))
		return ERR_PTR(-EINVAL);

	if (kprobe_disabled(p))
		return orig_p;

	/* Disable probe if it is a child probe */
	if (p != orig_p)
		p->flags |= KPROBE_FLAG_DISABLED;

	/* Try to disarm and disable this/parent probe */
	if (p == orig_p || aggr_kprobe_disabled(orig_p)) {
		/*
		 * Don't be lazy here.  Even if 'kprobes_all_disarmed'
		 * is false, 'orig_p' might not have been armed yet.
		 * Note arm_all_kprobes() __tries__ to arm all kprobes
		 * on the best effort basis.
		 */
		if (!kprobes_all_disarmed && !kprobe_disabled(orig_p)) {
			ret = disarm_kprobe(orig_p, true);
			if (ret) {
				p->flags &= ~KPROBE_FLAG_DISABLED;
				return ERR_PTR(ret);
			}
		}
		orig_p->flags |= KPROBE_FLAG_DISABLED;
	}

	return orig_p;
}
/* …（同文件无关代码省略）… */
static int __unregister_kprobe_top(struct kprobe *p)
{
	struct kprobe *ap, *list_p;

	/* Disable kprobe. This will disarm it if needed. */
	ap = __disable_kprobe(p);
	if (IS_ERR(ap))
		return PTR_ERR(ap);

	WARN_ON(ap != p && !kprobe_aggrprobe(ap));

	/*
	 * If the probe is an independent(and non-optimized) kprobe
	 * (not an aggrprobe), the last kprobe on the aggrprobe, or
	 * kprobe is already disarmed, just remove from the hash list.
	 */
	if (ap == p ||
		(list_is_singular(&ap->list) && kprobe_disarmed(ap))) {
		/*
		 * !disarmed could be happen if the probe is under delayed
		 * unoptimizing.
		 */
		hlist_del_rcu(&ap->hlist);
		return 0;
	}

	/* If disabling probe has special handlers, update aggrprobe */
	if (p->post_handler && !kprobe_gone(p)) {
		list_for_each_entry(list_p, &ap->list, list) {
			if ((list_p != p) && (list_p->post_handler))
				break;
		}
		/* No other probe has post_handler */
		if (list_entry_is_head(list_p, &ap->list, list)) {
			/*
			 * For the kprobe-on-ftrace case, we keep the
			 * post_handler setting to identify this aggrprobe
			 * armed with kprobe_ipmodify_ops.
			 */
			if (!kprobe_ftrace(ap))
				ap->post_handler = NULL;
		}
	}

	/*
	 * Remove from the aggrprobe: this path will do nothing in
	 * __unregister_kprobe_bottom().
	 */
	list_del_rcu(&p->list);
	if (!kprobe_disabled(ap) && !kprobes_all_disarmed)
		/*
		 * Try to optimize this probe again, because post
		 * handler may have been changed.
		 */
		optimize_kprobe(ap);
	return 0;

}
/* …（同文件无关代码省略）… */
static void __unregister_kprobe_bottom(struct kprobe *p)
{
	struct kprobe *ap;

	if (list_empty(&p->list))
		/* This is an independent kprobe */
		arch_remove_kprobe(p);
	else if (list_is_singular(&p->list)) {
		/* This is the last child of an aggrprobe */
		ap = list_entry(p->list.next, struct kprobe, list);
		list_del(&p->list);
		free_aggr_kprobe(ap);
	}
	/* Otherwise, do nothing. */
}
/* …（同文件无关代码省略）… */
static inline void free_rp_inst(struct kretprobe *rp)
{
	struct kretprobe_holder *rph = rp->rph;

	if (!rph)
		return;
	rp->rph = NULL;
	objpool_fini(&rph->pool);
}
/* …（同文件无关代码省略）… */
int kprobe_on_func_entry(kprobe_opcode_t *addr, const char *sym, unsigned long offset)
{
	bool on_func_entry;
	kprobe_opcode_t *kp_addr = _kprobe_addr(addr, sym, offset, &on_func_entry);

	if (IS_ERR(kp_addr))
		return PTR_ERR(kp_addr);

	if (!on_func_entry)
		return -EINVAL;

	return 0;
}
/* …（同文件无关代码省略）… */
	int ret;
	int i;
	void *addr;

	ret = kprobe_on_func_entry(rp->kp.addr, rp->kp.symbol_name, rp->kp.offset);
	if (ret)
		return ret;

	/* If only 'rp->kp.addr' is specified, check reregistering kprobes */
	if (rp->kp.addr && warn_kprobe_rereg(&rp->kp))
		return -EINVAL;

	if (kretprobe_blacklist_size) {
		addr = kprobe_addr(&rp->kp);
		if (IS_ERR(addr))
			return PTR_ERR(addr);

		for (i = 0; kretprobe_blacklist[i].name != NULL; i++) {
			if (kretprobe_blacklist[i].addr == addr)
				return -EINVAL;
/* …（同文件无关代码省略）… */
	for (i = 0; i < num; i++) {
		guard(mutex)(&kprobe_mutex);

		if (__unregister_kprobe_top(&rps[i]->kp) < 0)
			rps[i]->kp.addr = NULL;
#ifdef CONFIG_KRETPROBE_ON_RETHOOK
		rethook_free(rps[i]->rh);
#else
/* …（同文件无关代码省略）… */
	for (i = 0; i < num; i++) {
		if (rps[i]->kp.addr) {
			__unregister_kprobe_bottom(&rps[i]->kp);
#ifndef CONFIG_KRETPROBE_ON_RETHOOK
			free_rp_inst(rps[i]);
#endif
		}
