// AUTO-DRAFT from torvalds/linux PR #97d73c8251226cf27cc6656b4c542869bb8c69fa
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
		cpumask_set_cpu(smp_processor_id(), &amu_cpus);

		/* 0 reference values signal broken/disabled counters */
		if (!this_cpu_has_cap(ARM64_WORKAROUND_BROKEN_AMU_CONSTCNT))
			update_freq_counters_refs();
	}
}
/* …（同文件无关代码省略）… */
bool this_cpu_has_cap(unsigned int n)
{
	if (!WARN_ON(preemptible()) && n < ARM64_NCAPS) {
		const struct arm64_cpu_capabilities *cap = cpucap_ptrs[n];

		if (cap)
			return cap->matches(cap, SCOPE_LOCAL_CPU);
	}

	return false;
}
