// AUTO-DRAFT from torvalds/linux PR #d40cb8a12ed25789f39404c5f7968a65e98d89c3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

static int cpufreq_disabled(void)
{
	return off;
}
/* …（同文件无关代码省略）… */
static void adjust_jiffies(unsigned long val, struct cpufreq_freqs *ci)
{
#ifndef CONFIG_SMP
	static unsigned long l_p_j_ref;
	static unsigned int l_p_j_ref_freq;

	if (ci->flags & CPUFREQ_CONST_LOOPS)
		return;

	if (!l_p_j_ref_freq) {
		l_p_j_ref = loops_per_jiffy;
		l_p_j_ref_freq = ci->old;
		pr_debug("saving %lu as reference value for loops_per_jiffy; freq is %u kHz\n",
			 l_p_j_ref, l_p_j_ref_freq);
	}
	if (val == CPUFREQ_POSTCHANGE && ci->old != ci->new) {
		loops_per_jiffy = cpufreq_scale(l_p_j_ref, l_p_j_ref_freq,
								ci->new);
		pr_debug("scaling loops_per_jiffy to %lu for frequency %u kHz\n",
			 loops_per_jiffy, ci->new);
	}
#endif
}
/* …（同文件无关代码省略）… */
static void cpufreq_notify_transition(struct cpufreq_policy *policy,
				      struct cpufreq_freqs *freqs,
				      unsigned int state)
{
	int cpu;

	BUG_ON(irqs_disabled());

	if (cpufreq_disabled())
		return;

	freqs->policy = policy;
	freqs->flags = cpufreq_driver->flags;
	pr_debug("notification %u of frequency transition to %u kHz\n",
		 state, freqs->new);

	switch (state) {
	case CPUFREQ_PRECHANGE:
		/*
		 * Detect if the driver reported a value as "old frequency"
		 * which is not equal to what the cpufreq core thinks is
		 * "old frequency".
		 */
		if (policy->cur && policy->cur != freqs->old) {
			pr_debug("Warning: CPU frequency is %u, cpufreq assumed %u kHz\n",
				 freqs->old, policy->cur);
			freqs->old = policy->cur;
		}

		srcu_notifier_call_chain(&cpufreq_transition_notifier_list,
					 CPUFREQ_PRECHANGE, freqs);

		adjust_jiffies(CPUFREQ_PRECHANGE, freqs);
		break;

	case CPUFREQ_POSTCHANGE:
		adjust_jiffies(CPUFREQ_POSTCHANGE, freqs);
		pr_debug("FREQ: %u - CPUs: %*pbl\n", freqs->new,
			 cpumask_pr_args(policy->cpus));

		for_each_cpu(cpu, policy->cpus)
			trace_cpu_frequency(freqs->new, cpu);

		srcu_notifier_call_chain(&cpufreq_transition_notifier_list,
					 CPUFREQ_POSTCHANGE, freqs);

		cpufreq_stats_record_transition(policy, freqs->new);
		policy->cur = freqs->new;
	}
}
/* …（同文件无关代码省略）… */
static void cpufreq_notify_post_transition(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs, int transition_failed)
{
	cpufreq_notify_transition(policy, freqs, CPUFREQ_POSTCHANGE);
	if (!transition_failed)
		return;

	swap(freqs->old, freqs->new);
	cpufreq_notify_transition(policy, freqs, CPUFREQ_PRECHANGE);
	cpufreq_notify_transition(policy, freqs, CPUFREQ_POSTCHANGE);
}
/* …（同文件无关代码省略）… */
void cpufreq_freq_transition_begin(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs)
{

	/*
	 * Catch double invocations of _begin() which lead to self-deadlock.
	 * ASYNC_NOTIFICATION drivers are left out because the cpufreq core
	 * doesn't invoke _begin() on their behalf, and hence the chances of
	 * double invocations are very low. Moreover, there are scenarios
	 * where these checks can emit false-positive warnings in these
	 * drivers; so we avoid that by skipping them altogether.
	 */
	WARN_ON(!(cpufreq_driver->flags & CPUFREQ_ASYNC_NOTIFICATION)
				&& current == policy->transition_task);

wait:
	wait_event(policy->transition_wait, !policy->transition_ongoing);

	spin_lock(&policy->transition_lock);

	if (unlikely(policy->transition_ongoing)) {
		spin_unlock(&policy->transition_lock);
		goto wait;
	}

	policy->transition_ongoing = true;
	policy->transition_task = current;

	spin_unlock(&policy->transition_lock);

	cpufreq_notify_transition(policy, freqs, CPUFREQ_PRECHANGE);
}
/* …（同文件无关代码省略）… */
void cpufreq_freq_transition_end(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs, int transition_failed)
{
	if (WARN_ON(!policy->transition_ongoing))
		return;

	cpufreq_notify_post_transition(policy, freqs, transition_failed);

	arch_set_freq_scale(policy->related_cpus,
			    policy->cur,
			    arch_scale_freq_ref(policy->cpu));

	spin_lock(&policy->transition_lock);
	policy->transition_ongoing = false;
	policy->transition_task = NULL;
	spin_unlock(&policy->transition_lock);

	wake_up(&policy->transition_wait);
}
/* …（同文件无关代码省略）… */
static unsigned int __resolve_freq(struct cpufreq_policy *policy,
				   unsigned int target_freq,
				   unsigned int min, unsigned int max,
				   unsigned int relation, unsigned int *idx_p)
{
	unsigned int idx;

	target_freq = clamp_val(target_freq, min, max);

	if (!policy->freq_table)
		return target_freq;

	idx = cpufreq_frequency_table_target(policy, target_freq, min, max, relation);
	if (idx_p)
		*idx_p = idx;

	return policy->freq_table[idx].frequency;
}
/* …（同文件无关代码省略）… */
	for_each_cpu(cpu, mask) {
		i += sysfs_emit_at(buf, i, "%u ", cpu);
		if (i >= (PAGE_SIZE - 5))
			break;
	}
/* …（同文件无关代码省略）… */
static int __target_intermediate(struct cpufreq_policy *policy,
				 struct cpufreq_freqs *freqs, int index)
{
	int ret;

	freqs->new = cpufreq_driver->get_intermediate(policy, index);

	/* We don't need to switch to intermediate freq */
	if (!freqs->new)
		return 0;

	pr_debug("%s: cpu: %d, switching to intermediate freq: oldfreq: %u, intermediate freq: %u\n",
		 __func__, policy->cpu, freqs->old, freqs->new);

	cpufreq_freq_transition_begin(policy, freqs);
	ret = cpufreq_driver->target_intermediate(policy, index);
	cpufreq_freq_transition_end(policy, freqs, ret);

	if (ret)
		pr_err("%s: Failed to change to intermediate frequency: %d\n",
		       __func__, ret);

	return ret;
}
/* …（同文件无关代码省略）… */
static int __target_index(struct cpufreq_policy *policy, int index)
{
	struct cpufreq_freqs freqs = {.old = policy->cur, .flags = 0};
	unsigned int restore_freq, intermediate_freq = 0;
	unsigned int newfreq = policy->freq_table[index].frequency;
	int retval = -EINVAL;
	bool notify;

	if (newfreq == policy->cur)
		return 0;

	/* Save last value to restore later on errors */
	restore_freq = policy->cur;

	notify = !(cpufreq_driver->flags & CPUFREQ_ASYNC_NOTIFICATION);
	if (not
