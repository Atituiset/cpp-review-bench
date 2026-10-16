// AUTO-DRAFT from torvalds/linux PR #9aa622dbaab8addcbe6f65f17c7b4d613d270f05
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define KVM_TASK_SLEEP_HASHBITS 8
#define KVM_TASK_SLEEP_HASHSIZE (1<<KVM_TASK_SLEEP_HASHBITS)
/* …（同文件无关代码省略）… */
struct kvm_task_sleep_node {
	struct hlist_node link;
	struct swait_queue_head wq;
	u32 token;
	int cpu;
	bool dummy;
};
/* …（同文件无关代码省略）… */
static struct kvm_task_sleep_head {
	raw_spinlock_t lock;
	struct hlist_head list;
} async_pf_sleepers[KVM_TASK_SLEEP_HASHSIZE];
/* …（同文件无关代码省略）… */
static struct kvm_task_sleep_node *_find_apf_task(struct kvm_task_sleep_head *b,
						  u32 token)
{
	struct hlist_node *p;

	hlist_for_each(p, &b->list) {
		struct kvm_task_sleep_node *n =
			hlist_entry(p, typeof(*n), link);
		if (n->token == token)
			return n;
	}

	return NULL;
}
/* …（同文件无关代码省略）… */
static bool kvm_async_pf_queue_task(u32 token, struct kvm_task_sleep_node *n)
{
	u32 key = hash_32(token, KVM_TASK_SLEEP_HASHBITS);
	struct kvm_task_sleep_head *b = &async_pf_sleepers[key];
	struct kvm_task_sleep_node *e;

	raw_spin_lock(&b->lock);
	e = _find_apf_task(b, token);
	if (e) {
		struct kvm_task_sleep_node *dummy = NULL;

		/*
		 * The entry can either be a 'dummy' entry (which is put on the
		 * list when wake-up happens ahead of APF handling completion)
		 * or a token from another task which should not be touched.
		 */
		if (e->dummy) {
			hlist_del(&e->link);
			dummy = e;
		}

		raw_spin_unlock(&b->lock);
		kfree(dummy);
		return false;
	}

	n->token = token;
	n->cpu = smp_processor_id();
	n->dummy = false;
	init_swait_queue_head(&n->wq);
	hlist_add_head(&n->link, &b->list);
	raw_spin_unlock(&b->lock);
	return true;
}
/* …（同文件无关代码省略）… */
void kvm_async_pf_task_wait_schedule(u32 token)
{
	struct kvm_task_sleep_node n;
	DECLARE_SWAITQUEUE(wait);

	lockdep_assert_irqs_disabled();

	if (!kvm_async_pf_queue_task(token, &n))
		return;

	for (;;) {
		prepare_to_swait_exclusive(&n.wq, &wait, TASK_UNINTERRUPTIBLE);
		if (hlist_unhashed(&n.link))
			break;

		local_irq_enable();
		schedule();
		local_irq_disable();
	}
	finish_swait(&n.wq, &wait);
}
/* …（同文件无关代码省略）… */
noinstr u32 kvm_read_and_reset_apf_flags(void)
{
	u32 flags = 0;

	if (__this_cpu_read(async_pf_enabled)) {
		flags = __this_cpu_read(apf_reason.flags);
		__this_cpu_write(apf_reason.flags, 0);
	}

	return flags;
}
EXPORT_SYMBOL_FOR_KVM(kvm_read_and_reset_apf_flags);

noinstr bool __kvm_handle_async_pf(struct pt_regs *regs, u32 token)
{
	u32 flags = kvm_read_and_reset_apf_flags();
	irqentry_state_t state;

	if (!flags)
		return false;

/* …（同文件无关代码省略）… */
		panic("Host injected async #PF in interrupt disabled region\n");

	if (flags & KVM_PV_REASON_PAGE_NOT_PRESENT) {
		if (unlikely(!(user_mode(regs))))
			panic("Host injected async #PF in kernel mode\n");
		/* Page is swapped out by the host. */
		kvm_async_pf_task_wait_schedule(token);
	} else {
