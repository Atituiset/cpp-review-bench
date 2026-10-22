// AUTO-DRAFT from torvalds/linux PR #4403278e4ea2f654f8130a5c03eb8b4516a0f201
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
struct time_in_idle {
	u64 time;
	u64 timestamp;
};
/* …（同文件无关代码省略）… */
struct cpufreq_cooling_device {
	u32 last_load;
	unsigned int cpufreq_state;
	unsigned int max_level;
	struct em_perf_domain *em;
	struct cpufreq_policy *policy;
	struct thermal_cooling_device_ops cooling_ops;
#ifndef CONFIG_SMP
	struct time_in_idle *idle_time;
#endif
	struct freq_qos_request qos_req;
};
/* …（同文件无关代码省略）… */
static unsigned long get_level(struct cpufreq_cooling_device *cpufreq_cdev,
			       unsigned int freq)
{
	struct em_perf_state *table;
	int i;

	rcu_read_lock();
	table = em_perf_state_from_pd(cpufreq_cdev->em);
	for (i = cpufreq_cdev->max_level - 1; i >= 0; i--) {
		if (freq > table[i].frequency)
			break;
	}
	rcu_read_unlock();

	return cpufreq_cdev->max_level - i - 1;
}
/* …（同文件无关代码省略）… */
static u32 cpu_freq_to_power(struct cpufreq_cooling_device *cpufreq_cdev,
			     u32 freq)
{
	struct em_perf_state *table;
	unsigned long power_mw;
	int i;

	rcu_read_lock();
	table = em_perf_state_from_pd(cpufreq_cdev->em);
	for (i = cpufreq_cdev->max_level - 1; i >= 0; i--) {
		if (freq > table[i].frequency)
			break;
	}

	power_mw = table[i + 1].power;
	power_mw /= MICROWATT_PER_MILLIWATT;
	rcu_read_unlock();

	return power_mw;
}
/* …（同文件无关代码省略）… */
static u32 cpu_power_to_freq(struct cpufreq_cooling_device *cpufreq_cdev,
			     u32 power)
{
	struct em_perf_state *table;
	unsigned long em_power_mw;
	u32 freq;
	int i;

	rcu_read_lock();
	table = em_perf_state_from_pd(cpufreq_cdev->em);
	for (i = cpufreq_cdev->max_level; i > 0; i--) {
		/* Convert EM power to milli-Watts to make safe comparison */
		em_power_mw = table[i].power;
		em_power_mw /= MICROWATT_PER_MILLIWATT;
		if (power >= em_power_mw)
			break;
	}
	freq = table[i].frequency;
	rcu_read_unlock();

	return freq;
}
/* …（同文件无关代码省略）… */
static u32 get_load(struct cpufreq_cooling_device *cpufreq_cdev, int cpu,
		    int cpu_idx)
{
	unsigned long util = sched_cpu_util(cpu);

	return (util * 100) / arch_scale_cpu_capacity(cpu);
}
/* …（同文件无关代码省略）… */
static u32 get_dynamic_power(struct cpufreq_cooling_device *cpufreq_cdev,
			     unsigned long freq)
{
	u32 raw_cpu_power;

	raw_cpu_power = cpu_freq_to_power(cpufreq_cdev, freq);
	return (raw_cpu_power * cpufreq_cdev->last_load) / 100;
}
/* …（同文件无关代码省略）… */
static int cpufreq_get_requested_power(struct thermal_cooling_device *cdev,
				       u32 *power)
{
	unsigned long freq;
	int i = 0, cpu;
	u32 total_load = 0;
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;

	freq = cpufreq_quick_get(policy->cpu);

	for_each_cpu(cpu, policy->related_cpus) {
		u32 load;

		if (cpu_online(cpu))
			load = get_load(cpufreq_cdev, cpu, i);
		else
			load = 0;

		total_load += load;
	}

	cpufreq_cdev->last_load = total_load;

	*power = get_dynamic_power(cpufreq_cdev, freq);

	trace_thermal_power_cpu_get_power_simple(policy->cpu, *power);

	return 0;
}
/* …（同文件无关代码省略）… */
static int cpufreq_state2power(struct thermal_cooling_device *cdev,
			       unsigned long state, u32 *power)
{
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	unsigned int freq, num_cpus, idx;
	struct em_perf_state *table;

	/* Request state should be less than max_level */
	if (state > cpufreq_cdev->max_level)
		return -EINVAL;

	num_cpus = cpumask_weight(cpufreq_cdev->policy->cpus);

	idx = cpufreq_cdev->max_level - state;

	rcu_read_lock();
	table = em_perf_state_from_pd(cpufreq_cdev->em);
	freq = table[idx].frequency;
	rcu_read_unlock();

	*power = cpu_freq_to_power(cpufreq_cdev, freq) * num_cpus;

	return 0;
}
/* …（同文件无关代码省略）… */
static int cpufreq_power2state(struct thermal_cooling_device *cdev,
			       u32 power, unsigned long *state)
{
	unsigned int target_freq;
	u32 last_load, normalised_power;
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;

	last_load = cpufreq_cdev->last_load ?: 1;
	normalised_power = (power * 100) / last_load;
	target_freq = cpu_power_to_freq(cpufreq_cdev, normalised_power);

	*state = get_level(cpufreq_cdev, target_freq);
	trace_thermal_power_cpu_limit(policy->related_cpus, target_freq, *state,
				      power);
	return 0;
}
/* …（同文件无关代码省略）… */
static inline bool em_is_sane(struct cpufreq_cooling_device *cpufreq_cdev,
			      struct em_perf_domain *em) {
	struct cpufreq_policy *policy;
	unsigned int nr_levels;

	if (!em || em_is_artificial(em))
		return false;

	policy = cpufreq_cdev->policy;
	if (!cpumask_equal(policy->related_cpus, em_span_cpus(em))) {
		pr_err("The span of pd %*pbl is misaligned with cpufreq policy %*pbl\n",
			cpumask_pr_args(em_span_cpus(em)),
			cpumask_pr_args(policy->related_cpus));
		return false;
	}

	nr_levels = cpufreq_cdev->max_level + 1;
	if (em_pd_nr_perf_states(em) != nr_levels) {
		pr_err("The number of performance states in pd %*pbl (%u) doesn't match the number of cooling levels (%u)\n",
			cpumask_pr_args(em_span_cpus(em)),
			em_pd_nr_perf_states(em), nr_levels);
		return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static inline int allocate_idle_time(struct cpufreq_cooling_device *cpufreq_cdev)
{
	return 0;
}
/* …（同文件无关代码省略）… */
static unsigned int get_state_freq(struct cpufreq_cooling_device *cpufreq_cdev,
				   unsigned long state)
{
	struct cpufreq_policy *policy;
	unsigned long idx;

#ifdef CONFIG_THERMAL_GOV_POWER_ALLOCATOR
	/* Use the Energy Model table if available */
	if (cpufreq_cdev->em) {
		struct em_perf_state *table;
		unsigned int freq;

		idx = cpufreq_cdev->max_level - state;

		rcu_read_lock();
		table = em_perf_state_from_pd(cpufreq_cdev->em);
		freq = table[idx].frequency;
		rcu_read_unlock();

		return freq;
	}
#endif

	/* Otherwise, fallback on the CPUFreq table */
	policy = cpufreq_cdev->policy;
	if (policy->freq_table_sorted == CPUFREQ_TABLE_SORTED_ASCENDING)
		idx = cpufreq_cdev->max_level - state;
	else
		idx = state;

	return policy->freq_table[idx].frequency;
}
/* …（同文件无关代码省略）… */
static int cpufreq_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;

	*state = cpufreq_cdev->max_level;
	return 0;
}
/* …（同文件无关代码省略）… */
static int cpufreq_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;

	*state = cpufreq_cdev->cpufreq_state;

	return 0;
}
/* …（同文件无关代码省略）… */
static int cpufreq_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	unsigned int frequency;
	int ret;

	/* Request state should be less than max_level */
	if (state > cpufreq_cdev->max_level)
		return -EINVAL;

	/* Check if the old cooling action is same as new cooling action */
	if (cpufreq_cdev->cpufreq_state == state)
		return 0;

	frequency = get_state_freq(cpufreq_cdev, state);

	ret = freq_qos_update_request(&cpufreq_cdev->qos_req, frequency);
	if (ret >= 0) {
		cpufreq_cdev->cpufreq_state = state;
		ret = 0;
	}

	return ret;
}

/**
/* …（同文件无关代码省略）… */
static struct thermal_cooling_device *
__cpufreq_cooling_register(struct device_node *np,
			struct cpufreq_policy *policy,
			struct em_perf_domain *em)
{
	struct thermal_cooling_device *cdev;
	struct cpufreq_cooling_device *cpufreq_cdev;
	unsigned int i;
	struct device *dev;
	int ret;
	struct thermal_cooling_device_ops *cooling_ops;
	char *name;

	if (IS_ERR_OR_NULL(policy)) {
		pr_err("%s: cpufreq policy isn't valid: %p\n", __func__, policy);
		return ERR_PTR(-EINVAL);
	}

	dev = get_cpu_device(policy->cpu);
	if (unlikely(!dev)) {
		pr_warn("No cpu device for cpu %d\n", policy->cpu);
		return ERR_PTR(-ENODEV);
	}

	i = cpufreq_table_count_valid_entries(policy);
	if (!i) {
		pr_debug("%s: CPUFreq table not found or has no valid entries\n",
			 __func__);
		return ERR_PTR(-ENODEV);
	}

	cpufreq_cdev = kzalloc_obj(*cpufreq_cdev);
	if (!cpufreq_cdev)
		return ERR_PTR(-ENOMEM);

	cpufreq_cdev->policy = policy;

	ret = allocate_idle_time(cpufreq_cdev);
	if (ret) {
		cdev = ERR_PTR(ret);
		goto free_cdev;
	}

	/* max_level is an index, not a counter */
	cpufreq_cdev->max_level = i - 1;

	cooling_ops = &cpufreq_cdev->cooling_ops;
	cooling_ops->get_max_state = cpufreq_get_max_state;
	cooling_ops->get_cur_state = cpufreq_get_cur_state;
	cooling_ops->set_cur_state = cpufreq_set_cur_state;

#ifdef CONFIG_THERMAL_GOV_POWER_ALLOCATOR
	if (em_is_sane(cpufreq_cdev, em)) {
		cpufreq_cdev->em = em;
		cooling_ops->get_requested_power = cpufreq_get_requested_power;
		cooling_ops->state2power = cpufreq_state2power;
		cooling_ops->power2state = cpufreq_power2state;
	} else
#endif
	if (policy->freq_table_sorted == CPUFREQ_TABLE_UNSORTED) {
		pr_err("%s: unsorted frequency tables are not supported\n",
		       __func__);
		cdev = ERR_PTR(-EINVAL);
		goto free_idle_time;
	}

	ret = freq_qos_add_request(&policy->constraints,
				   &cpufreq_cdev->qos_req, FREQ_QOS_MAX,
				   get_state_freq(cpufreq_cdev, 0));
	if (ret < 0) {
		pr_err("%s: Failed to add freq constraint (%d)\n", __func__,
		       ret);
		cdev = ERR_PTR(ret);
		goto free_idle_time;
	}

	cdev = ERR_PTR(-ENOMEM);
	name = kasprintf(GFP_KERNEL, "cpufreq-%s", dev_name(dev));
	if (!name)
		goto remove_qos_req;

	cdev = thermal_of_cooling_device_register(np, 0, name, cpufreq_cdev,
						  cooling_ops);
	kfree(name);

	if (IS_ERR(cdev))
		goto remove_qos_req;

	return cdev;

remove_qos_req:
	freq_qos_remove_request(&cpufreq_cdev->qos_req);
free_idle_time:
	free_idle_time(cpufreq_cdev);
free_cdev:
	kfree(cpufreq_cdev);
	return cdev;
}
/* …（同文件无关代码省略）… */

		cdev = __cpufreq_cooling_register(np, policy, em);
		if (IS_ERR(cdev)) {
			pr_err("cpufreq_cooling: cpu%d failed to register as cooling device: %ld\n",
			       policy->cpu, PTR_ERR(cdev));
			cdev = NULL;
		}
	}
