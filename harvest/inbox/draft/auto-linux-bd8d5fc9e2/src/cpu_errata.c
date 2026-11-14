// AUTO-DRAFT from torvalds/linux PR #97d73c8251226cf27cc6656b4c542869bb8c69fa
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

static inline bool __is_midr_in_range(u32 midr, struct midr_range const *range)
{
	return midr_is_cpu_model_range(midr, range->model,
				       range->rv_min, range->rv_max);
}
/* …（同文件无关代码省略）… */
static inline bool is_midr_in_range(struct midr_range const *range)
{
	int i;

	if (!target_impl_cpu_num)
		return __is_midr_in_range(read_cpuid_id(), range);

	for (i = 0; i < target_impl_cpu_num; i++) {
		if (__is_midr_in_range(target_impl_cpus[i].midr, range))
			return true;
	}
	return false;
}
/* …（同文件无关代码省略）… */
bool is_midr_in_range_list(struct midr_range const *ranges)
{
	while (ranges->model)
		if (is_midr_in_range(ranges++))
			return true;
	return false;
}
/* …（同文件无关代码省略）… */
static bool __maybe_unused
is_affected_midr_range_list(const struct arm64_cpu_capabilities *entry,
			    int scope)
{
	WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());
	return is_midr_in_range_list(entry->midr_range_list);
}
/* …（同文件无关代码省略）… */
#define CAP_MIDR_RANGE_LIST(list)				\
	.matches = is_affected_midr_range_list,			\
	.midr_range_list = list
/* …（同文件无关代码省略）… */
	MIDR_RANGE(MIDR_CORTEX_A510, 0, 0, 1, 1),
#endif
#ifdef CONFIG_ARM64_ERRATUM_3821522
	/* Cortex-A725 r0p0 - r0p2 */
	MIDR_RANGE(MIDR_CORTEX_A725, 0, 0, 0, 2),
#endif
	{}
};
#endif /* CONFIG_ARM64_WORKAROUND_BROKEN_AMU_CONSTCNT */

#ifdef CONFIG_CAVIUM_ERRATUM_23154
static const struct midr_range cavium_erratum_23154_cpus[] = {
	MIDR_ALL_VERSIONS(MIDR_THUNDERX),
/* …（同文件无关代码省略）… */
		CAP_MIDR_RANGE_LIST(workaround_amu_constcnt_list)
	},
#endif
#ifdef CONFIG_ARM64_ERRATUM_2038923
	{
		.desc = "ARM erratum 2038923",
