// AUTO-DRAFT from torvalds/linux PR #100638f0f016cb0e6c75e95ff2b0495bca0b3054
  // <<< BUG ANCHOR
	cpu = cpumask_first(policy->related_cpus);
	max_freq = arch_scale_freq_ref(cpu);
	if (!max_freq)
		max_freq = policy->cpuinfo.max_freq;

	capped_freq = policy->max;
