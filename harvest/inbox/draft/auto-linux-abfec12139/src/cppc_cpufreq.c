// AUTO-DRAFT from torvalds/linux PR #d40cb8a12ed25789f39404c5f7968a65e98d89c3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void cppc_cpufreq_put_cpu_data(struct cpufreq_policy *policy)
{
	struct cppc_cpudata *cpu_data = policy->driver_data;

	free_cpumask_var(cpu_data->shared_cpu_map);
	kfree(cpu_data);
	policy->driver_data = NULL;
}
/* …（同文件无关代码省略）… */
		pr_debug("Unsupported CPU co-ord type: %d\n",
			 policy->shared_type);
		ret = -EFAULT;
		goto out;
	}

	policy->fast_switch_possible = cppc_allow_fast_switch(policy->cpus);
/* …（同文件无关代码省略）… */
	return 0;

out:
	cppc_cpufreq_put_cpu_data(policy);
	return ret;
}
