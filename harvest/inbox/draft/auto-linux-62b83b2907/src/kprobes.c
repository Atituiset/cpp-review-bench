// AUTO-DRAFT from torvalds/linux PR #1ffd6efb1b58e596ed70f660182803264a4c84d9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	 */
	op = container_of(ap, struct optimized_kprobe, kp);
	WARN_ON_ONCE(list_empty(&op->list));
	/* Enable the probe again */
	ap->flags &= ~KPROBE_FLAG_DISABLED;
	/* Optimize it again. (remove from 'op->list') */
/* …（同文件无关代码省略）… */
#define unoptimize_kprobe(p, f)			do {} while (0)
/* …（同文件无关代码省略）… */
/* Add the new probe to 'ap->list'. */
static int add_new_kprobe(struct kprobe *ap, struct kprobe *p)
{
	if (p->post_handler)
		unoptimize_kprobe(ap, true);	/* Fall back to normal kprobe */

	list_add_rcu(&p->list, &ap->list);
