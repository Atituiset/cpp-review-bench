// AUTO-DRAFT from torvalds/linux PR #df37e3924292fb41d325721008520ba93deced42
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
 */

#include <linux/bitfield.h>

#include <acpi/cppc_acpi.h>
#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/topology.h>

#define CPPC_HIGHEST_PERF_PERFORMANCE	196
#define CPPC_HIGHEST_PERF_PREFCORE	166
/* …（同文件无关代码省略）… */
	return true;
}

int cpc_read_ffh(int cpunum, struct cpc_reg *reg, u64 *val)
{
	int err;

	err = rdmsrq_safe_on_cpu(cpunum, reg->address, val);
	if (!err) {
		u64 mask = GENMASK_ULL(reg->bit_offset + reg->bit_width - 1,
/* …（同文件无关代码省略）… */
	u64 rd_val;
	int err;

	err = rdmsrq_safe_on_cpu(cpunum, reg->address, &rd_val);
	if (!err) {
		u64 mask = GENMASK_ULL(reg->bit_offset + reg->bit_width - 1,
/* …（同文件无关代码省略）… */

static void amd_set_max_freq_ratio(void)
{
	struct cppc_perf_caps perf_caps;
	u64 numerator, nominal_perf;
	u64 perf_ratio;
	int rc;

	rc = cppc_get_perf_caps(0, &perf_caps);
	if (rc) {
		pr_debug("Could not retrieve perf counters (%d)\n", rc);
		return;
	}

	rc = amd_get_boost_ratio_numerator(0, &numerator);
	if (rc) {
		pr_debug("Could not retrieve highest performance (%d)\n", rc);
		return;
	}
	nominal_perf = perf_caps.nominal_perf;

	if (!nominal_perf) {
		pr_debug("Could not retrieve nominal performance\n");
		return;
	}

	/* midpoint between max_boost and max_P */
	perf_ratio = (div_u64(numerator * SCHED_CAPACITY_SCALE, nominal_perf) + SCHED_CAPACITY_SCALE) >> 1;

	freq_invariance_set_perf_ratio(perf_ratio, false);
}
/* …（同文件无关代码省略）… */
int amd_get_highest_perf(unsigned int cpu, u32 *highest_perf)
{
	u64 val;
	int ret;

	if (cpu_feature_enabled(X86_FEATURE_CPPC)) {
		ret = rdmsrq_safe_on_cpu(cpu, MSR_AMD_CPPC_CAP1, &val);
		if (ret)
			goto out;

		val = FIELD_GET(AMD_CPPC_HIGHEST_PERF_MASK, val);
	} else {
		ret = cppc_get_highest_perf(cpu, &val);
		if (ret)
			goto out;
	}

	WRITE_ONCE(*highest_perf, (u32)val);
out:
	return ret;
}
/* …（同文件无关代码省略）… */
int amd_detect_prefcore(bool *detected)
{
	int cpu, count = 0;
	u64 highest_perf[2] = {0};

	if (WARN_ON(!detected))
		return -EINVAL;

	switch (amd_pref_core_detected) {
	case AMD_PREF_CORE_SUPPORTED:
		*detected = true;
		return 0;
	case AMD_PREF_CORE_UNSUPPORTED:
		*detected = false;
		return 0;
	default:
		break;
	}

	for_each_online_cpu(cpu) {
		u32 tmp;
		int ret;

		ret = amd_get_highest_perf(cpu, &tmp);
		if (ret)
			return ret;

		if (!count || (count == 1 && tmp != highest_perf[0]))
			highest_perf[count++] = tmp;

		if (count == 2)
			break;
	}

	*detected = (count == 2);
	boost_numerator = highest_perf[0];

	amd_pref_core_detected = *detected ? AMD_PREF_CORE_SUPPORTED :
					     AMD_PREF_CORE_UNSUPPORTED;

	pr_debug("AMD CPPC preferred core is %ssupported (highest perf: 0x%llx)\n",
		 *detected ? "" : "un", highest_perf[0]);

	return 0;
}
EXPORT_SYMBOL_GPL(amd_detect_prefcore);

/**
 * amd_get_boost_ratio_numerator: Get the numerator to use for boost ratio calculation
 * @cpu: CPU to get numerator for.
 * @numerator: Output variable for numerator.
 *
 * Determine the numerator to use for calculating the boost ratio on
 * a CPU. On systems that support preferred cores, this will be a hardcoded
 * value. On other systems this will the highest performance register value.
 *
 * If booting the system with amd-pstate enabled but preferred cores disabled then
 * the correct boost numerator will be returned to match hardware capabilities
 * even if the preferred cores scheduling hints are not enabled.
 *
 * Return: 0 for success, negative error code otherwise.
 */
int amd_get_boost_ratio_numerator(unsigned int cpu, u64 *numerator)
{
	bool prefcore;
	int ret;
	u32 tmp;

	ret = amd_detect_prefcore(&prefcore);
	if (ret)
		return ret;

	/* without preferred cores, return the highest perf register value */
	if (!prefcore) {
		*numerator = boost_numerator;
		return 0;
	}

	/*
	 * For AMD CPUs with Family ID 19H and Model ID range 0x70 to 0x7f,
/* …（同文件无关代码省略）… */
	if (cpu_feature_enabled(X86_FEATURE_ZEN4)) {
		switch (boot_cpu_data.x86_model) {
		case 0x70 ... 0x7f:
			*numerator = CPPC_HIGHEST_PERF_PERFORMANCE;
			return 0;
		default:
			break;
		}
	}

	/* detect if running on heterogeneous design */
	if (cpu_feature_enabled(X86_FEATURE_AMD_HTR_CORES)) {
		switch (cpu_data(cpu).topo.cpu_type) {
		case TOPO_CPU_TYPE_UNKNOWN:
		case TOPO_CPU_TYPE_ANY:
			pr_warn("Undefined core type found for cpu %d\n", cpu);
			break;
		case TOPO_CPU_TYPE_PERFORMANCE:
			/* use the max scale for performance cores */
			*numerator = CPPC_HIGHEST_PERF_PERFORMANCE;
			return 0;
		case TOPO_CPU_TYPE_LOW_POWER:
		case TOPO_CPU_TYPE_EFFICIENCY:
			/* use the highest perf value for efficiency and low-power cores */
			ret = amd_get_highest_perf(cpu, &tmp);
			if (ret)
				return ret;
			*numerator = tmp;
			return 0;
		}
	}

	*numerator = CPPC_HIGHEST_PERF_PREFCORE;

	return 0;
}
EXPORT_SYMBOL_GPL(amd_get_boost_ratio_numerator);
