// AUTO-DRAFT from torvalds/linux PR #e767a4ea70a3992c37ed604157d32f0dfbf9b1e3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
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
