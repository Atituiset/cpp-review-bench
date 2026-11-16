// AUTO-DRAFT from torvalds/linux PR #a9daf327f3ff90625204876981f88cfe2b6f6fff
  // <<< BUG ANCHOR
	for_each_cpu(cpu, llc_cpus) {
		nr_cpus++;
		if (cpumask_first(topology_sibling_cpumask(cpu)) == cpu)
			nr_cores++;
	}
