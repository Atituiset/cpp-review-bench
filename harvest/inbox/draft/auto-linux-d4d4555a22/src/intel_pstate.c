// AUTO-DRAFT from torvalds/linux PR #100638f0f016cb0e6c75e95ff2b0495bca0b3054
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

struct pstate_data {
	int	current_pstate;
	int	min_pstate;
	int	max_pstate;
	int	max_pstate_physical;
	int	perf_ctl_scaling;
	int	scaling;
	int	turbo_pstate;
	unsigned int min_freq;
	unsigned int max_freq;
	unsigned int turbo_freq;
};
/* …（同文件无关代码省略）… */
struct vid_data {
	int min;
	int max;
	int turbo;
	int32_t ratio;
};
/* …（同文件无关代码省略）… */
struct cpudata {
	int cpu;

	unsigned int policy;
	struct update_util_data update_util;
	bool   update_util_set;

	struct pstate_data pstate;
	struct vid_data vid;

	u64	last_update;
	u64	last_sample_time;
	u64	aperf_mperf_shift;
	u64	prev_aperf;
	u64	prev_mperf;
	u64	prev_tsc;
	struct sample sample;
	int32_t	min_perf_ratio;
	int32_t	max_perf_ratio;
#ifdef CONFIG_ACPI
	struct acpi_processor_performance acpi_perf_data;
	bool valid_pss_table;
#endif
	unsigned int iowait_boost;
	s16 epp_powersave;
	s16 epp_policy;
	s16 epp_default;
	s16 epp_cached;
	u64 hwp_req_cached;
	u64 hwp_cap_cached;
	u64 last_io_update;
	unsigned int capacity_perf;
	unsigned int sched_flags;
	u32 hwp_boost_min;
	bool suspended;
#ifdef CONFIG_ENERGY_MODEL
	bool pd_registered;
#endif
	struct delayed_work hwp_notify_work;
};
/* …（同文件无关代码省略）… */
struct pstate_funcs {
	int (*get_max)(int cpu);
	int (*get_max_physical)(int cpu);
	int (*get_min)(int cpu);
	int (*get_turbo)(int cpu);
	int (*get_scaling)(void);
	int (*get_cpu_scaling)(int cpu);
	int (*get_aperf_mperf_shift)(void);
	u64 (*get_val)(struct cpudata*, int pstate);
	void (*get_vid)(struct cpudata *);
};
/* …（同文件无关代码省略）… */
static bool intel_pstate_acpi_pm_profile_server(void)
{
	if (acpi_gbl_FADT.preferred_profile == PM_ENTERPRISE_SERVER ||
	    acpi_gbl_FADT.preferred_profile == PM_PERFORMANCE_SERVER)
		return true;

	return false;
}
/* …（同文件无关代码省略）… */
static bool intel_pstate_get_ppc_enable_status(void)
{
	if (intel_pstate_acpi_pm_profile_server())
		return true;

	return acpi_ppc;
}
/* …（同文件无关代码省略）… */
#define CPPC_MAX_PERF	U8_MAX
/* …（同文件无关代码省略）… */
static void intel_pstate_set_itmt_prio(int cpu)
{
	struct cppc_perf_caps cppc_perf;
	static u32 max_highest_perf = 0, min_highest_perf = U32_MAX;
	int ret;

	ret = cppc_get_perf_caps(cpu, &cppc_perf);
	/*
	 * If CPPC is not available, fall back to MSR_HWP_CAPABILITIES bits [8:0].
	 *
	 * Also, on some systems with overclocking enabled, CPPC.highest_perf is
	 * hardcoded to 0xff, so CPPC.highest_perf cannot be used to enable ITMT.
	 * Fall back to MSR_HWP_CAPABILITIES then too.
	 */
	if (ret || cppc_perf.highest_perf == CPPC_MAX_PERF)
		cppc_perf.highest_perf = HWP_HIGHEST_PERF(READ_ONCE(all_cpu_data[cpu]->hwp_cap_cached));

	/*
	 * The priorities can be set regardless of whether or not
	 * sched_set_itmt_support(true) has been called and it is valid to
	 * update them at any time after it has been called.
	 */
	sched_set_itmt_core_prio(cppc_perf.highest_perf, cpu);

	if (max_highest_perf <= min_highest_perf) {
		if (cppc_perf.highest_perf > max_highest_perf)
			max_highest_perf = cppc_perf.highest_perf;

		if (cppc_perf.highest_perf < min_highest_perf)
			min_highest_perf = cppc_perf.highest_perf;

		if (max_highest_perf > min_highest_perf) {
			/*
			 * This code can be run during CPU online under the
			 * CPU hotplug locks, so sched_set_itmt_support()
			 * cannot be called from here.  Queue up a work item
			 * to invoke it.
			 */
			schedule_work(&sched_itmt_work);
		}
	}
}
/* …（同文件无关代码省略）… */
static void intel_pstate_init_acpi_perf_limits(struct cpufreq_policy *policy)
{
	struct cpudata *cpu;
	int ret;
	int i;

	if (hwp_active) {
		intel_pstate_set_itmt_prio(policy->cpu);
		return;
	}

	if (!intel_pstate_get_ppc_enable_status())
		return;

	cpu = all_cpu_data[policy->cpu];

	ret = acpi_processor_register_performance(&cpu->acpi_perf_data,
						  policy->cpu);
	if (ret)
		return;

	/*
	 * Check if the control value in _PSS is for PERF_CTL MSR, which should
	 * guarantee that the states returned by it map to the states in our
	 * list directly.
	 */
	if (cpu->acpi_perf_data.control_register.space_id !=
						ACPI_ADR_SPACE_FIXED_HARDWARE)
		goto err;

	/*
	 * If there is only one entry _PSS, simply ignore _PSS and continue as
	 * usual without taking _PSS into account
	 */
	if (cpu->acpi_perf_data.state_count < 2)
		goto err;

	pr_debug("CPU%u - ACPI _PSS perf data\n", policy->cpu);
	for (i = 0; i < cpu->acpi_perf_data.state_count; i++) {
		pr_debug("     %cP%d: %u MHz, %u mW, 0x%x\n",
			 (i == cpu->acpi_perf_data.state ? '*' : ' '), i,
			 (u32) cpu->acpi_perf_data.states[i].core_frequency,
			 (u32) cpu->acpi_perf_data.states[i].power,
			 (u32) cpu->acpi_perf_data.states[i].control);
	}

	cpu->valid_pss_table = true;
	pr_debug("_PPC limits will be enforced\n");

	return;

 err:
	cpu->valid_pss_table = false;
	acpi_processor_unregister_performance(policy->cpu);
}
/* …（同文件无关代码省略）… */
static void intel_pstate_exit_perf_limits(struct cpufreq_policy *policy)
{
	struct cpudata *cpu;

	cpu = all_cpu_data[policy->cpu];
	if (!cpu->valid_pss_table)
		return;

	acpi_processor_unregister_performance(policy->cpu);
}
/* …（同文件无关代码省略）… */
static s16 intel_pstate_get_epp(struct cpudata *cpu_data, u64 hwp_req_data)
{
	s16 epp = -EOPNOTSUPP;

	if (boot_cpu_has(X86_FEATURE_HWP_EPP)) {
		/*
		 * When hwp_req_data is 0, means that caller didn't read
		 * MSR_HWP_REQUEST, so need to read and get EPP.
		 */
		if (!hwp_req_data) {
			epp = rdmsrq_on_cpu(cpu_data->cpu, MSR_HWP_REQUEST,
					    &hwp_req_data);
			if (epp)
				return epp;
		}
		epp = (hwp_req_data >> 24) & 0xff;
	}

	return epp;
}
/* …（同文件无关代码省略）… */
static int intel_pstate_set_epp(struct cpudata *cpu, u32 epp)
{
	int ret;

	/*
	 * Use the cached HWP Request MSR value, because in the active mode the
	 * register itself may be updated by intel_pstate_hwp_boost_up() or
	 * intel_pstate_hwp_boost_down() at any time.
	 */
	u64 value = READ_ONCE(cpu->hwp_req_cached);

	value &= ~GENMASK_ULL(31, 24);
	val
