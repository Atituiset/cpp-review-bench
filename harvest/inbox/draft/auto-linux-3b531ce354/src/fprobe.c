// AUTO-DRAFT from torvalds/linux PR #e767a4ea70a3992c37ed604157d32f0dfbf9b1e3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define FPROBE_HASH_BITS 6
/* …（同文件无关代码省略）… */
#define SIZE_IN_LONG(x) ((x + sizeof(long) - 1) >> (sizeof(long) == 8 ? 3 : 2))
/* …（同文件无关代码省略）… */
static DEFINE_MUTEX(fprobe_mutex);
static struct fgraph_ops fprobe_graph_ops;

static u32 fprobe_node_hashfn(const void *data, u32 len, u32 seed)
{
	return hash_ptr(*(unsigned long **)data, 32);
/* …（同文件无关代码省略）… */
static bool fprobe_registered(struct fprobe *fp)
{
	struct hlist_head *head;
	struct fprobe_hlist *fph;

	head = &fprobe_table[hash_ptr(fp, FPROBE_HASH_BITS)];
	hlist_for_each_entry_rcu(fph, head, hlist,
				 lockdep_is_held(&fprobe_mutex)) {
		if (fph->fp == fp)
			return true;
	}
	return false;
}
/* …（同文件无关代码省略）… */
#define FPROBE_HEADER_SIZE_IN_LONG	1
/* …（同文件无关代码省略）… */
static inline void read_fprobe_header(unsigned long *stack,
					struct fprobe **fp, unsigned int *size_words)
{
	if (!*stack) {
		*fp = NULL;
		*size_words = 0;
		return;
	}
	*fp = arch_decode_fprobe_header_fp(*stack);
	*size_words = arch_decode_fprobe_header_size(*stack);
}
/* …（同文件无关代码省略）… */
static inline int __fprobe_handler(unsigned long ip, unsigned long parent_ip,
				   struct fprobe *fp, struct ftrace_regs *fregs,
				   void *data)
{
	if (!fp->entry_handler)
		return 0;

	return fp->entry_handler(fp, ip, parent_ip, fregs, data);
}
/* …（同文件无关代码省略）… */
static inline int __fprobe_kprobe_handler(unsigned long ip, unsigned long parent_ip,
					  struct fprobe *fp, struct ftrace_regs *fregs,
					  void *data)
{
	int ret;
	/*
	 * This user handler is shared with other kprobes and is not expected to be
	 * called recursively. So if any other kprobe handler is running, this will
	 * exit as kprobe does. See the section 'Share the callbacks with kprobes'
	 * in Documentation/trace/fprobe.rst for more information.
	 */
	if (unlikely(kprobe_running())) {
		fp->nmissed++;
		return 0;
	}

	kprobe_busy_begin();
	ret = __fprobe_handler(ip, parent_ip, fp, fregs, data);
	kprobe_busy_end();
	return ret;
}
/* …（同文件无关代码省略）… */
static void fprobe_ftrace_entry(unsigned long ip, unsigned long parent_ip,
	struct ftrace_ops *ops, struct ftrace_regs *fregs)
{
	struct fprobe_hlist_node *node;
	struct rhlist_head *head, *pos;
	struct fprobe *fp;
	int bit;

	bit = ftrace_test_recursion_trylock(ip, parent_ip);
	if (bit < 0)
		return;

	/*
	 * ftrace_test_recursion_trylock() disables preemption, but
	 * rhltable_lookup() checks whether rcu_read_lcok is held.
	 * So we take rcu_read_lock() here.
	 */
	rcu_read_lock();
	head = rhltable_lookup(&fprobe_ip_table, &ip, fprobe_rht_params);

	rhl_for_each_entry_rcu(node, pos, head, hlist) {
		if (node->addr != ip)
			break;
		fp = READ_ONCE(node->fp);
		if (unlikely(!fp || fprobe_disabled(fp) || fp->exit_handler))
			continue;

		if (fprobe_shared_with_kprobes(fp))
			__fprobe_kprobe_handler(ip, parent_ip, fp, fregs, NULL);
		else
			__fprobe_handler(ip, parent_ip, fp, fregs, NULL);
	}
	rcu_read_unlock();
	ftrace_test_recursion_unlock(bit);
}
NOKPROBE_SYMBOL(fprobe_ftrace_entry);
/* …（同文件无关代码省略）… */
	struct fprobe *fp;
	int used, ret;

	if (WARN_ON_ONCE(!fregs))
		return 0;

	guard(rcu)();
	head = rhltable_lookup(&fprobe_ip_table, &func, fprobe_rht_params);
	reserved_words = 0;
	rhl_for_each_entry_rcu(node, pos, head, hlist) {
/* …（同文件无关代码省略）… */
static void fprobe_return(struct ftrace_graph_ret *trace,
			  struct fgraph_ops *gops,
			  struct ftrace_regs *fregs)
{
	unsigned long *fgraph_data = NULL;
	unsigned long ret_ip;
	struct fprobe *fp;
	int size, curr;
	int size_words;

	fgraph_data = (unsigned long *)fgraph_retrieve_data(gops->idx, &size);
	if (WARN_ON_ONCE(!fgraph_data))
		return;
	size_words = SIZE_IN_LONG(size);
	ret_ip = ftrace_regs_get_instruction_pointer(fregs);

	preempt_disable_notrace();

	curr = 0;
	while (size_words > curr) {
		read_fprobe_header(&fgraph_data[curr], &fp, &size);
		if (!fp)
			break;
		curr += FPROBE_HEADER_SIZE_IN_LONG;
		if (fprobe_registered(fp) && !fprobe_disabled(fp)) {
			if (WARN_ON_ONCE(curr + size > size_words))
				break;
			fp->exit_handler(fp, trace->func, ret_ip, fregs,
					 size ? fgraph_data + curr : NULL);
		}
		curr += size;
	}
	preempt_enable_notrace();
}
NOKPROBE_SYMBOL(fprobe_return);
