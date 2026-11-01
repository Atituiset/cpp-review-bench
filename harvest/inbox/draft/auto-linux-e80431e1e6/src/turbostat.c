// AUTO-DRAFT from torvalds/linux PR #cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define UNUSED(x) (void)(x)
/* …（同文件无关代码省略）… */
#define	NAME_BYTES 20
#define PATH_BYTES 128
#define PERF_NAME_BYTES 128
/* …（同文件无关代码省略）… */
#define PERF_DEV_NAME_BYTES 32
#define PERF_EVT_NAME_BYTES 32
/* …（同文件无关代码省略）… */

#define ROUND_UP_TO_PAGE_SIZE(n) (((n) + 0x1000UL-1UL) & ~(0x1000UL-1UL))

enum counter_scope { SCOPE_CPU, SCOPE_CORE, SCOPE_PACKAGE };
enum counter_type { COUNTER_ITEMS, COUNTER_CYCLES, COUNTER_SECONDS, COUNTER_USEC, COUNTER_K2M };
enum counter_format { FORMAT_RAW, FORMAT_DELTA, FORMAT_PERCENT, FORMAT_AVERAGE };
enum counter_source { COUNTER_SOURCE_NONE, COUNTER_SOURCE_PERF, COUNTER_SOURCE_MSR };
/* …（同文件无关代码省略）… */
struct perf_counter_info {
	struct perf_counter_info *next;

	/* How to open the counter / What counter it is. */
	char device[PERF_DEV_NAME_BYTES];
	char event[PERF_EVT_NAME_BYTES];

	/* How to show/format the counter. */
	char name[PERF_NAME_BYTES];
	unsigned int width;
	enum counter_scope scope;
	enum counter_type type;
	enum counter_format format;
	double scale;

	/* For reading the counter. */
	int *fd_perf_per_domain;
	size_t num_domains;
};
/* …（同文件无关代码省略）… */
struct msr_counter {
	unsigned int msr_num;
	char name[NAME_BYTES];
	struct sysfs_path *sp;
	unsigned int width;
	enum counter_type type;
	enum counter_format format;
	struct msr_counter *next;
	unsigned int flags;
#define	FLAGS_HIDE	(1 << 0)
#define	FLAGS_SHOW	(1 << 1)
#define	SYSFS_PERCPU	(1 << 1)
};
static int use_android_msr_path;
/* …（同文件无关代码省略）… */
	MAX_BIC
};

void print_bic_set(char *s, cpu_set_t *set)
{
	int i;

	assert(MAX_BIC < CPU_SETSIZE);

	printf("%s:", s);

	for (i = 0; i < MAX_BIC; ++i) {

		if (CPU_ISSET(i, set))
			printf(" %s", bic[i].name);
	}
	putchar('\n');
}

static cpu_set_t bic_group_topology;
static cpu_set_t bic_group_thermal_pwr;
static cpu_set_t bic_group_frequency;
static cpu_set_t bic_group_hw_idle;
static cpu_set_t bic_group_sw_idle;
static cpu_set_t bic_group_idle;
static cpu_set_t bic_group_cache;
static cpu_set_t bic_group_other;
static cpu_set_t bic_group_disabled_by_default;
static cpu_set_t bic_enabled;
static cpu_set_t bic_present;

/* modify */
#define BIC_INIT(set) CPU_ZERO(set)
/* …（同文件无关代码省略）… */
#define SET_BIC(COUNTER_NUMBER, set) CPU_SET(COUNTER_NUMBER, set)
#define CLR_BIC(COUNTER_NUMBER, set) CPU_CLR(COUNTER_NUMBER, set)
/* …（同文件无关代码省略）… */
#define BIC_NOT_PRESENT(COUNTER_NUMBER) CPU_CLR(COUNTER_NUMBER, &bic_present)
/* …（同文件无关代码省略）… */
#define DO_BIC_READ(COUNTER_NUMBER) CPU_ISSET(COUNTER_NUMBER, &bic_present)
#define DO_BIC(COUNTER_NUMBER) (CPU_ISSET(COUNTER_NUMBER, &bic_enabled) && CPU_ISSET(COUNTER_NUMBER, &bic_present))

static void bic_set_all(cpu_set_t *set)
{
	int i;

	assert(MAX_BIC < CPU_SETSIZE);

	for (i = 0; i < MAX_BIC; ++i)
		SET_BIC(i, set);
}
/* …（同文件无关代码省略）… */
 * bic_clear_bits()
 * clear all the bits from "clr" in "dst"
 */
static void bic_clear_bits(cpu_set_t *dst, cpu_set_t *clr)
{
	int i;

/* …（同文件无关代码省略）… */
	SET_BIC(BIC_CorWatt, &bic_group_thermal_pwr);
	SET_BIC(BIC_GFXWatt, &bic_group_thermal_pwr);
	SET_BIC(BIC_RAMWatt, &bic_group_thermal_pwr);
	SET_BIC(BIC_PKG__, &bic_group_thermal_pwr);
	SET_BIC(BIC_RAM__, &bic_group_thermal_pwr);
	SET_BIC(BIC_SysWatt, &bic_group_thermal_pwr);
/* …（同文件无关代码省略）… */
#define PCLUNL 15		/* Unlimited */

char *proc_stat = "/proc/stat";
FILE *outf;
int *fd_percpu;
int *fd_instr_count_percpu;
int *fd_llc_percpu;
/* …（同文件无关代码省略）… */
unsigned int summary_only;
unsigned int list_header_only;
unsigned int dump_only;
unsigned int force_load;
unsigned int cpuid_has_aperf_mperf;
unsigned int cpuid_has_hv;
unsigned int has_aperf_access;
/* …（同文件无关代码省略）… */
struct gfx_sysfs_info {
	FILE *fp;
	unsigned int val;
	unsigned long long val_ull;
};

static struct gfx_sysfs_info gfx_info[GFX_MAX];

int get_msr(int cpu, off_t offset, unsigned long long *msr);
int add_counter(unsigned int msr_num, char *path, char *name,
		unsigned int width, enum counter_scope scope, enum counter_type type, enum counter_format format, int flags, int package_num);

/* Model specific support Start */

/* …（同文件无关代码省略）… */
#define SLM_BCLK_FREQS 5
/* …（同文件无关代码省略）… */
	unsigned int i;
	double freq;

	if (get_msr(master_cpu, MSR_FSB_FREQ, &msr))
		fprintf(outf, "SLM BCLK: unknown\n");

	i = msr & 0xf;
	if (i >= SLM_BCLK_FREQS) {
/* …（同文件无关代码省略）… */
	freq = slm_freq_table[i];

	if (!quiet)
		fprintf(outf, "SLM BCLK: %.1f Mhz\n", freq);

	return freq;
}
/* …（同文件无关代码省略）… */
#define RAPL_PKG	(RAPL_PKG_ENERGY_STATUS | RAPL_PKG_POWER_LIMIT)
#define RAPL_DRAM	(RAPL_DRAM_ENERGY_STATUS | RAPL_DRAM_POWER_LIMIT)
/* …（同文件无关代码省略）… */
#define RAPL_GFX	(RAPL_GFX_POWER_LIMIT | RAPL_GFX_ENERGY_STATUS)
/* …（同文件无关代码省略）… */
#define RAPL_AMD_F17H	(RAPL_AMD_PWR_UNIT | RAPL_AMD_CORE_ENERGY_STAT | RAPL_AMD_PKG_ENERGY_STAT)
/* …（同文件无关代码省略）… */
	{ INTEL_ARROWLAKE, &adl_features },
	{ INTEL_LUNARLAKE_M, &lnl_features },
	{ INTEL_PANTHERLAKE_L, &lnl_features },
	{ INTEL_NOVALAKE, &lnl_features },
	{ INTEL_NOVALAKE_L, &lnl_features },
	{ INTEL_WILDCATLAKE_L, &lnl_features },
/* …（同文件无关代码省略）… */
struct perf_l2_events {
	unsigned long long refs;	/* L2_REQUEST.ALL */
	unsigned long long hits;	/* L2_REQUEST.HIT */
};
/* …（同文件无关代码省略）… */
struct perf_model_support {
	unsigned int vfm;
	struct perf_l2_events first;
	struct perf_l2_events second;
	struct perf_l2_events third;
} *perf_model_support;
/* …（同文件无关代码省略）… */
#define	PCE(ext_umask, umask)	(((unsigned long long) ext_umask) << 40 | umask << 8 | 0x24)
/* …（同文件无关代码省略）… */
	{ INTEL_ATOM_CRESTMONT, { PCE(0x00, 0x00), PCE(0x00, 0x02)}, {}, {} },	/* GRR */
	{ INTEL_ATOM_DARKMONT_X, { PCE(0x01, 0xFF), PCE(0x01, 0xBF)}, {}, {} },	/* CWF */

	{ INTEL_ALDERLAKE, { PCE(0x00, 0xFF), PCE(0x00, 0xDF)}, { PCE(0x00, 0x00), PCE(0x00, 0x02)}, {} },
	{ INTEL_ALDERLAKE, { PCE(0x00, 0xFF), PCE(0x00, 0xDF)}, { PCE(0x00, 0x00), PCE(0x00, 0x02)}, {} },
	{ INTEL_ALDERLAKE_L, { PCE(0x00, 0xFF), PCE(0x00, 0xDF)}, { PCE(0x00, 0x00), PCE(0x00, 0x02)}, {} },
	{ INTEL_RAPTORLAKE, { PCE(0x00, 0xFF), PCE(0x00, 0xDF)}, { PCE(0x00, 0x00), PCE(0x00, 0x02)}, {} },
/* …（同文件无关代码省略）… */
	{ INTEL_ARROWLAKE, { PCE(0x00, 0xFF), PCE(0x00, 0x5F)}, { PCE(0x00, 0x07), PCE(0x00, 0x02)}, {} },

	{ INTEL_PANTHERLAKE_L, { PCE(0x00, 0xFF), PCE(0x00, 0x5F)}, { PCE(0x01, 0xFF), PCE(0x01, 0xBF)}, {} },
	{ INTEL_WILDCATLAKE_L, { PCE(0x00, 0xFF), PCE(0x00, 0x5F)}, { PCE(0x01, 0xFF), PCE(0x01, 0xBF)}, {} },

	{ INTEL_NOVALAKE, { PCE(0x00, 0xFF), PCE(0x00, 0x5F)}, { PCE(0x01, 0xFF), PCE(0x01, 0xBF)}, {} },
/* …（同文件无关代码省略）… */

	if (authentic_amd || hygon_genuine) {
		/* fallback to default features on unsupported models */
		force_load++;
		if (max_extended_level >= 0x80000007) {
			unsigned int eax, ebx, ecx, edx;

/* …（同文件无关代码省略）… */
	}

end:
	if (force_load && !platform) {
		fprintf(outf, "Forced to run on unsupported platform!\n");
		platform = &default_features;
	}
/* …（同文件无关代码省略）… */
#define MSR_RAPL_PWR_UNIT	0xc0010299
/* …（同文件无关代码省略）… */
#define MSR_PKG_ENERGY_STAT	0xc001029b
/* …（同文件无关代码省略）… */
int backwards_count;
char *progname;

#define CPU_SUBSET_MAXCPUS	8192	/* need to use before probe... */
cpu_set_t *cpu_present_set, *cpu_possible_set, *cpu_effective_set, *cpu_allowed_set, *cpu_affinity_set, *cpu_subset;
cpu_set_t *perf_pcore_set, *perf_ecore_set, *perf_lcore_set;
size_t cpu_present_setsize, cpu_possible_setsize, cpu_effective_setsize, cpu_allowed_setsize, cpu_affinity_setsize, cpu_subset_size;
#define MAX_ADDED_THREAD_COUNTERS 24
#define MAX_ADDED_CORE_COUNTERS 8
#define MAX_ADDED_PACKAGE_COUNTERS 16
#define PMT_MAX_ADDED_THREAD_COUNTERS 24
/* …（同文件无关代码省略）… */
#define PMT_MAX_ADDED_PACKAGE_COUNTERS 16
#define BITMASK_SIZE 32
/* …（同文件无关代码省略）… */
#define ZERO_ARRAY(arr) (memset(arr, 0, sizeof(arr)) + __must_be_array(arr))
/* …（同文件无关代码省略）… */
enum rapl_unit {
	RAPL_UNIT_INVALID,
	RAPL_UNIT_JOULES,
	RAPL_UNIT_WATTS,
};
/* …（同文件无关代码省略）… */
struct rapl_counter_info_t {
	unsigned long long data[NUM_RAPL_COUNTERS];
	enum counter_source source[NUM_RAPL_COUNTERS];
	unsigned long long flags[NUM_RAPL_COUNTERS];
	double scale[NUM_RAPL_COUNTERS];
	enum rapl_unit unit[NUM_RAPL_COUNTERS];
	unsigned long long msr[NUM_RAPL_COUNTERS];
	unsigned long long msr_mask[NUM_RAPL_COUNTERS];
	int msr_shift[NUM_RAPL_COUNTERS];

	int fd_perf;
};
/* …（同文件无关代码省略）… */
#define RAPL_COUNTER_FLAG_USE_MSR_SUM (1u << 1)
/* …（同文件无关代码省略）… */
struct rapl_counter {
	unsigned long long raw_value;
	enum rapl_unit unit;
	double scale;
};
/* …（同文件无关代码省略）… */
struct cstate_counter_info_t {
	unsigned long long data[NUM_CSTATE_COUNTERS];
	enum counter_source source[NUM_CSTATE_COUNTERS];
	unsigned long long msr[NUM_CSTATE_COUNTERS];
	int fd_perf_core;
	int fd_perf_pkg;
};
/* …（同文件无关代码省略）… */
struct msr_counter_info_t {
	unsigned long long data[NUM_MSR_COUNTERS];
	enum counter_source source[NUM_MSR_COUNTERS];
	unsigned long long msr[NUM_MSR_COUNTERS];
	unsigned long long msr_mask[NUM_MSR_COUNTERS];
	int fd_perf;
};
/* …（同文件无关代码省略）… */
#define SYSFS_TELEM_PATH "/sys/class/intel_pmt"
/* …（同文件无关代码省略）… */
#define PMT_COUNTER_CWF_MC1E_LSB                  0
#define PMT_COUNTER_CWF_MC1E_MSB                  63
#define PMT_CWF_MC1E_GUID                         0x14421519
/* …（同文件无关代码省略）… */
#define PMT_COUNTER_NAME_SIZE_BYTES      16
/* …（同文件无关代码省略）… */
struct pmt_mmio {
	struct pmt_mmio *next;

	unsigned int guid;
	unsigned int size;

	/* Base pointer to the mmaped memory. */
	void *mmio_base;

	/*
	 * Offset to be applied to the mmio_base
	 * to get the beginning of the PMT counters for given GUID.
	 */
	unsigned long pmt_offset;
} *pmt_mmios;
/* …（同文件无关代码省略）… */
enum pmt_datatype {
	PMT_TYPE_RAW,
	PMT_TYPE_XTAL_TIME,
	PMT_TYPE_TCORE_CLOCK,
};
/* …（同文件无关代码省略）… */
struct pmt_domain_info {
	/*
	 * Pointer to the MMIO obtained by applying a counter offset
	 * to the mmio_base of the mmaped region for the given GUID.
	 *
	 * This is where to read the raw value of the counter from.
	 */
	unsigned long *pcounter;
};
/* …（同文件无关代码省略）… */
struct pmt_counter {
	struct pmt_counter *next;

	/* PMT metadata */
	char name[PMT_COUNTER_NAME_SIZE_BYTES];
	enum pmt_datatype type;
	enum counter_scope scope;
	unsigned int lsb;
	unsigned int msb;

	/* BIC-like metadata */
	enum counter_format format;

	unsigned int num_domains;
	struct pmt_domain_info *domains;
};
/* …（同文件无关代码省略）… */
struct pmt_diriter_t {
	DIR *dir;
	struct dirent **namelist;
	unsigned int num_names;
	unsigned int current_name_idx;
};
/* …（同文件无关代码省略）… */
{
	unsigned int dummy;

	return sscanf(e->d_name, "telem%u", &dummy);
}

int pmt_telemdir_sort(const struct dirent **a, const struct dirent **b)
{
	unsigned int aidx = 0, bidx = 0;

	sscanf((*a)->d_name, "telem%u", &aidx);
	sscanf((*b)->d_name, "telem%u", &bidx);

	return (aidx > bidx) ? 1 : (aidx < bidx) ? -1 : 0;
}
/* …（同文件无关代码省略）… */
const struct dirent *pmt_diriter_begin(struct pmt_diriter_t *iter, const char *pmt_root_path)
{
	int num_names = iter->num_names;

	if (!iter->dir) {
		iter->dir = opendir(pmt_root_path);
		if (iter->dir == NULL)
			return NULL;

		num_names = scandir(pmt_root_path, &iter->namelist, pmt_telemdir_filter, pmt_telemdir_sort);
		if (num_names == -1) {
			closedir(iter->dir);
			iter->dir = NULL;
			return NULL;
		}
	}

	iter->current_name_idx = 0;
	iter->num_names = num_names;

	return pmt_diriter_next(iter);
}
/* …（同文件无关代码省略）… */
void pmt_diriter_init(struct pmt_diriter_t *iter)
{
	memset(iter, 0, sizeof(*iter));
}
/* …（同文件无关代码省略）… */
void pmt_diriter_remove(struct pmt_diriter_t *iter)
{
	if (iter->namelist) {
		for (unsigned int i = 0; i < iter->num_names; i++) {
			free(iter->namelist[i]);
			iter->namelist[i] = NULL;
		}
	}

	free(iter->namelist);
	iter->namelist = NULL;
	iter->num_names = 0;
	iter->current_name_idx = 0;

	if (iter->dir) {
		closedir(iter->dir);
		iter->dir = NULL;
	}
}
/* …（同文件无关代码省略）… */
struct llc_stats {
	unsigned long long references;
	unsigned long long misses;
};
struct l2_stats {
	unsigned long long references;
	unsigned long long hits;
};
struct thread_data {
	struct timeval tv_begin;
	struct timeval tv_end;
	struct timeval tv_delta;
	unsigned long long tsc;
	unsigned long long aperf;
	unsigned long long mperf;
	unsigned long long c1;
	unsigned long long instr_count;
	unsigned long long irq_count;
	unsigned long long nmi_count;
	unsigned int smi_count;
	struct llc_stats llc;
	struct l2_stats l2;
	unsigned int cpu_id;
	unsigned int apic_id;
	unsigned int x2apic_id;
	unsigned int flags;
	bool is_atom;
	unsigned long long counter[MAX_ADDED_THREAD_COUNTERS];
	unsigned long long perf_counter[MAX_ADDED_THREAD_COUNTERS];
	unsigned long long pmt_counter[PMT_MAX_ADDED_THREAD_COUNTERS];
};
/* …（同文件无关代码省略）… */
struct pkg_data {
	int first_cpu;
	unsigned long long pc2;
	unsigned long long pc3;
	unsigned long long pc6;
	unsigned long long pc7;
	unsigned long long pc8;
	unsigned long long pc9;
	unsigned long long pc10;
	long long cpu_lpi;
	long long sys_lpi;
	unsigned long long pkg_wtd_core_c0;
	unsigned long long pkg_any_core_c0;
	unsigned long long pkg_any_gfxe_c0;
	unsigned long long pkg_both_core_gfxe_c0;
	long long gfx_rc6_ms;
	unsigned int gfx_mhz;
	unsigned int gfx_act_mhz;
	long long sam_mc6_ms;
	unsigned int sam_mhz;
	unsigned int sam_act_mhz;
	struct rapl_counter energy_pkg;	/* MSR_PKG_ENERGY_STATUS */
	struct rapl_counter energy_dram;	/* MSR_DRAM_ENERGY_STATUS */
	struct rapl_counter energy_cores;	/* MSR_PP0_ENERGY_STATUS */
	struct rapl_counter energy_gfx;	/* MSR_PP1_ENERGY_STATUS */
	struct rapl_counter rapl_pkg_perf_status;	/* MSR_PKG_PERF_STATUS */
	struct rapl_counter rapl_dram_perf_status;	/* MSR_DRAM_PERF_STATUS */
	unsigned int pkg_temp_c;
	unsigned int uncore_mhz;
	unsigned long long die_c6;
	unsigned long long counter[MAX_ADDED_PACKAGE_COUNTERS];
	unsigned long long perf_counter[MAX_ADDED_PACKAGE_COUNTERS];
	unsigned long long pmt_counter[PMT_MAX_ADDED_PACKAGE_COUNTERS];
};
/* …（同文件无关代码省略）… */
#define ODD_COUNTERS odd.threads, odd.cores, odd.packages
#define EVEN_COUNTERS even.threads, even.cores, even.packages
/* …（同文件无关代码省略）… */
off_t idx_to_offset(int idx)
{
	off_t offset;

	switch (idx) {
	case IDX_PKG_ENERGY:
		if (platform->plat_rapl_msrs & RAPL_AMD_F17H)
			offset = MSR_PKG_ENERGY_STAT;
		else
			offset = MSR_PKG_ENERGY_STATUS;
		break;
	case IDX_DRAM_ENERGY:
		offset = MSR_DRAM_ENERGY_STATUS;
		break;
	case IDX_PP0_ENERGY:
		offset = MSR_PP0_ENERGY_STATUS;
		break;
	case IDX_PP1_ENERGY:
		offset = MSR_PP1_ENERGY_STATUS;
		break;
	case IDX_PKG_PERF:
		offset = MSR_PKG_PERF_STATUS;
		break;
	case IDX_DRAM_PERF:
		offset = MSR_DRAM_PERF_STATUS;
		break;
	case IDX_PSYS_ENERGY:
		offset = MSR_PLATFORM_ENERGY_STATUS;
		break;
	default:
		offset = -1;
	}
	return offset;
}
/* …（同文件无关代码省略）… */
struct counters {
	struct thread_data *threads;
	struct core_data *cores;
	struct pkg_data *packages;
} average, even, odd;
/* …（同文件无关代码省略）… */
struct platform_counters {
	struct rapl_counter energy_psys;	/* MSR_PLATFORM_ENERGY_STATUS */
} platform_counters_odd, platform_counters_even;
/* …（同文件无关代码省略）… */
#define	MAX_HT_ID	3	/* support SMT-4 */
/* …（同文件无关代码省略）… */
	if (cpu < 0)
		return 1;

	return !CPU_ISSET_S(cpu, cpu_present_setsize, cpu_present_set);
}

int cpu_is_not_allowed(int cpu)
{
	if (cpu < 0)
		return 1;

	return !CPU_ISSET_S(cpu, cpu_allowed_setsize, cpu_allowed_set);
}

#define GLOBAL_CORE_ID(core_id, pkg_id)	(core_id + pkg_id * (topo.max_core_id + 1))
/* …（同文件无关代码省略）… */
#define PER_THREAD_PARAMS  struct thread_data *t, struct core_data *c, struct pkg_data *p
/* …（同文件无关代码省略）… */
	return 0;
}

int for_all_cpus(int (func) (struct thread_data *, struct core_data *, struct pkg_data *),
		 struct thread_data *thread_base, struct core_data *core_base, struct pkg_data *pkg_base)
{
	int cpu, retval;

/* …（同文件无关代码省略）… */
int is_cpu_first_thread_in_core(struct thread_data *t, struct core_data *c)
{
	return ((int)t->cpu_id == c->first_cpu || c->first_cpu < 0);
}
/* …（同文件无关代码省略）… */
int is_cpu_first_core_in_package(struct thread_data *t, struct pkg_data *p)
{
	return ((int)t->cpu_id == p->first_cpu || p->first_cpu < 0);
}
/* …（同文件无关代码省略）… */

int cpu_migrate(int cpu)
{
	CPU_ZERO_S(cpu_affinity_setsize, cpu_affinity_set);
	CPU_SET_S(cpu, cpu_affinity_setsize, cpu_affinity_set);
	if (sched_setaffinity(0, cpu_affinity_setsize, cpu_affinity_set) == -1)
		return -1;
	else
		return 0;
/* …（同文件无关代码省略）… */
int get_msr_fd(int cpu)
{
	char pathname[32];
	int fd;

	fd = fd_percpu[cpu];

	if (fd)
		return fd;
	sprintf(pathname, use_android_msr_path ? "/dev/msr%d" : "/dev/cpu/%d/msr", cpu);
	fd = open(pathname, O_RDONLY);
	if (fd < 0)
		err(-1, "%s open failed, try chown or chmod +r %s, "
		    "or run with --no-msr, or run as root", pathname, use_android_msr_path ? "/dev/msr*" : "/dev/cpu/*/msr");
	fd_percpu[cpu] = fd;

	return fd;
}
/* …（同文件无关代码省略）… */
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags)
{
	assert(!no_perf);

	return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}
/* …（同文件无关代码省略）… */
static long open_perf_counter(int cpu, unsigned int type, unsigned int config, int group_fd, __u64 read_format)
{
	struct perf_event_attr attr;
	const pid_t pid = -1;
	const unsigned long flags = 0;

	assert(!no_perf);

	memset(&attr, 0, sizeof(struct perf_event_attr));

	attr.type = type;
	attr.size = sizeof(struct perf_event_attr);
	attr.config = config;
	attr.disabled = 0;
	attr.sample_type = PERF_SAMPLE_IDENTIFIER;
	attr.read_format = read_format;

	const int fd = perf_event_open(&attr, pid, cpu, group_fd, flags);

	return fd;
}
/* …（同文件无关代码省略）… */
int get_instr_count_fd(int cpu)
{
	if (fd_instr_count_percpu[cpu])
		return fd_instr_count_percpu[cpu];

	fd_instr_count_percpu[cpu] = open_perf_counter(cpu, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, -1, 0);

	return fd_instr_count_percpu[cpu];
}

int get_msr(int cpu, off_t offset, unsigned long long *msr)
{
	ssize_t retval;

	assert(!no_msr);

	retval = pread(get_msr_fd(cpu), msr, sizeof(*msr), offset);

	if (retval != sizeof *msr)
		err(-1, "cpu%d: msr offset 0x%llx read failed", cpu, (unsigned long long)offset);

	return 0;
}

int add_msr_counter(int cpu, off_t offset)
{
	ssize_t retval;
	unsigned long long value;

	if (no_msr)
		return -1;

	if (!offset)
		return -1;

	retval = pread(get_msr_fd(cpu), &value, sizeof(value), offset);

	/* if the read failed, the probe fails */
	if (retval != sizeof(value))
		return -1;

	if (value == 0)
		return 0;

	return 1;
}
/* …（同文件无关代码省略）… */
unsigned int cpu_to_domain(const struct perf_counter_info *pc, int cpu)
{
	switch (pc->scope) {
	case SCOPE_CPU:
		return cpu;

	case SCOPE_CORE:
		return cpus[cpu].core_id;

	case SCOPE_PACKAGE:
		return cpus[cpu].package_id;
	}

	__builtin_unreachable();
}
/* …（同文件无关代码省略）… */
enum show_hide_mode { SHOW_LIST, HIDE_LIST } global_show_hide_mode = HIDE_LIST;
/* …（同文件无关代码省略）… */
void help(void)
{
	fprintf(outf,
		"Usage: turbostat [OPTIONS][(--interval seconds) | COMMAND ...]\n"
		"\n"
		"Turbostat forks the specified COMMAND and prints statistics\n"
		"when COMMAND completes.\n"
		"If no COMMAND is specified, turbostat wakes every 5-seconds\n"
		"to print statistics, until interrupted.\n"
		"  -a, --add counter\n"
		"		add a counter\n"
		"		  eg. --add msr0x10,u64,cpu,delta,MY_TSC\n"
		"		  eg. --add perf/cstate_pkg/c2-residency,package,delta,percent,perfPC2\n"
		"		  eg. --add pmt,name=XTAL,type=raw,domain=package0,offset=0,lsb=0,msb=63,guid=0x1a067102\n"
		"  -c, --cpu cpu-set\n"
		"		limit output to summary plus cpu-set:\n"
		"		  {core | package | j,k,l..m,n-p }\n"
		"  -d, --debug\n"
		"		displays usec, Time_Of_Day_Seconds and more debugging\n"
		"		debug messages are printed to stderr\n"
		"  -D, --Dump\n"
		"		displays the raw counter values\n"
		"  -e, --enable [all | column]\n"
		"		shows all or the specified disabled column\n"
		"  -f, --force\n"
		"		force load turbostat with minimum default features on unsupported platforms.\n"
		"  -H, --hide [column | column,column,...]\n"
		"		hide the specified column(s)\n"
		"  -i, --interval sec.subsec\n"
		"		override default 5-second measurement interval\n"
		"  -J, --Joules\n"
		"		displays energy in Joules instead of Watts\n"
		"  -l, --list\n"
		"		list column headers only\n"
		"  -M, --no-msr\n"
		"		disable all uses of the MSR driver\n"
		"  -P, --no-perf\n"
		"		disable all uses of the perf API\n"
		"  -n, --num_iterations num\n"
		"		number of the measurement iterations\n"
		"  -N, --header_iterations num\n"
		"		print header every num iterations\n"
		"  -o, --out file\n"
		"		create or truncate \"file\" for all output\n"
		"  -q, --quiet\n"
		"		skip decoding system configuration header\n"
		"  -s, --show [column | column,column,...]\n"
		"		show only the specified column(s)\n"
		"  -S, --Summary\n"
		"		limits output to 1-line system summary per interval\n"
		"  -T, --TCC temperature\n"
		"		sets the Thermal Control Circuit temperature in\n"
		"		  degrees Celsius\n"
		"  -h, --help\n"
		"		print this help message\n  -v, --version\n\t\tprint version information\n\nFor more help, run \"man turbostat\"\n");
}

/*
 * bic_lookup
 * for all the strings in comma separate name_list,
 * set the approprate bit in return value.
 */
void bic_lookup(cpu_set_t *ret_set, char *name_list, enum show_hide_mode mode)
{
	unsigned int i;

/* …（同文件无关代码省略）… */
double pct(double numerator, double denominator)
{
	double retval;

	if (numerator < 0)
		return nan("");

	if (denominator <= 0)
		return nan("");

	retval = 100.0 * numerator / denominator;

	if (retval > 110.0)
		return nan("");

	return retval;
}
/* …（同文件无关代码省略）… */
int dump_counters(PER_THREAD_PARAMS)
{
	int i;
	struct msr_counter *mp;
	struct platform_counters *pplat_cnt = p == odd.packages ? &platform_counters_odd : &platform_counters_even;

	outp += sprintf(outp, "t %p, c %p, p %p\n", t, c, p);

	if (t) {
		outp += sprintf(outp, "CPU: %d flags 0x%x\n", t->cpu_id, t->flags);
		outp += sprintf(outp, "TSC: %016llX\n", t->tsc);
		outp += sprintf(outp, "aperf: %016llX\n", t->aperf);
		outp += sprintf(outp, "mperf: %016llX\n", t->mperf);
		outp += sprintf(outp, "c1: %016llX\n", t->c1);

		if (DO_BIC(BIC_IPC))
			outp += sprintf(outp, "IPC: %lld\n", t->instr_count);

		if (DO_BIC(BIC_IRQ))
			outp += sprintf(outp, "IRQ: %lld\n", t->irq_count);
		if (DO_BIC(BIC_NMI))
			outp += sprintf(outp, "IRQ: %lld\n", t->nmi_count);
		if (DO_BIC(BIC_SMI))
			outp += sprintf(outp, "SMI: %d\n", t->smi_count);

		outp += sprintf(outp, "LLC refs: %lld", t->llc.references);
		outp += sprintf(outp, "LLC miss: %lld", t->llc.misses);
		outp += sprintf(outp, "LLC Hit%%: %.2f", pct((t->llc.references - t->llc.misses), t->llc.references));

		outp += sprintf(outp, "L2 refs: %lld", t->l2.references);
		outp += sprintf(outp, "L2 hits: %lld", t->l2.hits);
		outp += sprintf(outp, "L2 Hit%%: %.2f", pct(t->l2.hits, t->l2.references));

		for (i = 0, mp = sys.tp; mp; i++, mp = mp->next) {
			outp += sprintf(outp, "tADDED [%d] %8s msr0x%x: %08llX %s\n", i, mp->name, mp->msr_num, t->counter[i], mp->sp->path);
		}
	}

	if (c && is_cpu_first_thread_in_core(t, c)) {
		outp += sprintf(outp, "core: 0x%x\n", cpus[t->cpu_id].core_id);
		outp += sprintf(outp, "c3: %016llX\n", c->c3);
		outp += sprintf(outp, "c6: %016llX\n", c->c6);
		outp += sprintf(outp, "c7: %016llX\n", c->c7);
		outp += sprintf(outp, "DTS: %dC\n", c->core_temp_c);
		outp += sprintf(outp, "cpu_throt_count: %016llX\n", c->core_throt_cnt);

		const unsigned long long energy_value = c->core_energy.raw_value * c->core_energy.scale;
		const double energy_scale = c->core_energy.scale;

		if (c->core_energy.unit == RAPL_UNIT_JOULES)
			outp += sprintf(outp, "Joules: %0llX (scale: %lf)\n", energy_value, energy_scale);

		for (i = 0, mp = sys.cp; mp; i++, mp = mp->next) {
			outp += sprintf(outp, "cADDED [%d] %8s msr0x%x: %08llX %s\n", i, mp->name, mp->msr_num, c->counter[i], mp->sp->path);
		}
		outp += sprintf(outp, "mc6_us: %016llX\n", c->mc6_us);
	}

	if (p && is_cpu_first_core_in_package(t, p)) {
		outp += sprintf(outp, "Weighted cores: %016llX\n", p->pkg_wtd_core_c0);
		outp += sprintf(outp, "Any cores: %016llX\n", p->pkg_any_core_c0);
		outp += sprintf(outp, "Any GFX: %016llX\n", p->pkg_any_gfxe_c0);
		outp += sprintf(outp, "CPU + GFX: %016llX\n", p->pkg_both_core_gfxe_c0);

		outp += sprintf(outp, "pc2: %016llX\n", p->pc2);
		if (DO_BIC(BIC_Pkgpc3))
			outp += sprintf(outp, "pc3: %016llX\n", p->pc3);
		if (DO_BIC(BIC_Pkgpc6))
			outp += sprintf(outp, "pc6: %016llX\n", p->pc6);
		if (DO_BIC(BIC_Pkgpc7))
			outp += sprintf(outp, "pc7: %016llX\n", p->pc7);
		outp += sprintf(outp, "pc8: %016llX\n", p->pc8);
		outp += sprintf(outp, "pc9: %016llX\n", p->pc9);
		outp += sprintf(outp, "pc10: %016llX\n", p->pc10);
		outp += sprintf(outp, "cpu_lpi: %016llX\n", p->cpu_lpi);
		outp += sprintf(outp, "sys_lpi: %016llX\n", p->sys_lpi);
		outp += sprintf(outp, "Joules PKG: %0llX\n", p->energy_pkg.raw_value);
		outp += sprintf(outp, "Joules COR: %0llX\n", p->energy_cores.raw_value);
		outp += sprintf(outp, "Joules GFX: %0llX\n", p->energy_gfx.raw_value);
		outp += sprintf(outp, "Joules RAM: %0llX\n", p->energy_dram.raw_value);
		outp += sprintf(outp, "Joules PSYS: %0llX\n", pplat_cnt->energy_psys.raw_value);
		outp += sprintf(outp, "Throttle PKG: %0llX\n", p->rapl_pkg_perf_status.raw_value);
		outp += sprintf(outp, "Throttle RAM: %0llX\n", p->rapl_dram_perf_status.raw_value);
		outp += sprintf(outp, "PTM: %dC\n", p->pkg_temp_c);

		for (i = 0, mp = sys.pp; mp; i++, mp = mp->next) {
			outp += sprintf(outp, "pADDED [%d] %8s msr0x%x: %08llX %s\n", i, mp->name, mp->msr_num, p->counter[i], mp->sp->path);
		}
	}

	outp += sprintf(outp, "\n");

	return 0;
}

double rapl_counter_get_value(const struct rapl_counter *c, enum rapl_unit desired_unit, double interval)
{
	assert(desired_unit != RAPL_UNIT_INVALID);

	/*
	 * For now we don't expect anything other than joules,
	 * so just simplify the logic.
/* …（同文件无关代码省略）… */
void get_perf_llc_stats(int cpu, struct llc_stats *llc)
{
	struct read_format {
		unsigned long long num_read;
		struct llc_stats llc;
	} r;
	const ssize_t expected_read_size = sizeof(r);
	ssize_t actual_read_size;

	actual_read_size = read(fd_llc_percpu[cpu], &r, expected_read_size);

	if (actual_read_size == -1)
		err(-1, "%s(cpu%d,) %d,,%ld", __func__, cpu, fd_llc_percpu[cpu], expected_read_size);

	llc->references = r.llc.references;
	llc->misses = r.llc.misses;
	if (actual_read_size != expected_read_size)
		warn("%s: failed to read perf_data (req %zu act %zu)", __func__, expected_read_size, actual_read_size);
}
/* …（同文件无关代码省略）… */
void get_perf_l2_stats(int cpu, struct l2_stats *l2)
{
	struct read_format {
		unsigned long long num_read;
		struct l2_stats l2;
	} r;
	const ssize_t expected_read_size = sizeof(r);
	ssize_t actual_read_size;

	actual_read_size = read(fd_l2_percpu[cpu], &r, expected_read_size);

	if (actual_read_size == -1)
		err(-1, "%s(cpu%d,) %d,,%ld", __func__, cpu, fd_l2_percpu[cpu], expected_read_size);

	l2->references = r.l2.references;
	l2->hits = r.l2.hits;
	if (actual_read_size != expected_read_size)
		warn("%s: cpu%d: failed to read(%d) perf_data (req %zu act %zu)", __func__, cpu, fd_l2_percpu[cpu], expected_read_size, actual_read_size);
}
/* …（同文件无关代码省略）… */
		return 0;

	/*if not summary line and --cpu is used */
	if ((t != average.threads) && (cpu_subset && !CPU_ISSET_S(t->cpu_id, cpu_subset_size, cpu_subset)))
		return 0;

	if (DO_BIC(BIC_USEC)) {
/* …（同文件无关代码省略）… */
void flush_output_stdout(void)
{
	FILE *filep;

	if (outf == stderr)
		filep = stdout;
	else
		filep = outf;

	fputs(output_buffer, filep);
	fflush(filep);

	outp = output_buffer;
}
/* …（同文件无关代码省略）… */
#define DELTA_WRAP32(new, old)			\
	old = ((((unsigned long long)new << 32) - ((unsigned long long)old << 32)) >> 32);
/* …（同文件无关代码省略）… */

	/* check for TSC < 1 Mcycles over interval */
	if (old->tsc < (1000 * 1000))
		errx(-3, "Insanely slow TSC rate, TSC stops in idle?\n"
		     "You can disable all c-states by booting with \"idle=poll\"\nor just the deep ones with \"processor.max_cstate=1\"");

	old->c1 = new->c1 - old->c1;

/* …（同文件无关代码省略）… */
static unsigned long long rdtsc(void)
{
	unsigned int low, high;

	asm volatile ("rdtsc":"=a" (low), "=d"(high));

	return low | ((unsigned long long)high) << 32;
}
/* …（同文件无关代码省略）… */
FILE *fopen_or_die(const char *path, const char *mode)
{
	FILE *filep = fopen(path, mode);

	if (!filep)
		err(1, "%s: open failed", path);
	return filep;
}
/* …（同文件无关代码省略）… */
unsigned long long snapshot_sysfs_counter(char *path)
{
	FILE *fp;
	int retval;
	unsigned long long counter;

	fp = fopen_or_die(path, "r");

	retval = fscanf(fp, "%lld", &counter);
	if (retval != 1)
		err(1, "snapshot_sysfs_counter(%s)", path);

	fclose(fp);

	return counter;
}
/* …（同文件无关代码省略）… */
int get_mp(int cpu, struct msr_counter *mp, unsigned long long *counterp, char *counter_path)
{
	if (mp->msr_num != 0) {
		assert(!no_msr);
		if (get_msr(cpu, mp->msr_num, counterp))
			return -1;
	} else {
		char path[128 + PATH_BYTES];

		if (mp->flags & SYSFS_PERCPU) {
			sprintf(path, "/sys/devices/system/cpu/cpu%d/%s", cpu, mp->sp->path);

			*counterp = snapshot_sysfs_counter(path);
		} else {
			*counterp = snapshot_sysfs_counter(counter_path);
		}
	}

	return 0;
}
/* …（同文件无关代码省略）… */
unsigned long long get_legacy_uncore_mhz(int package)
{
	char path[128];
	int die;
	static int warn_once;

	/*
	 * for this package, use the first die_id that exists
	 */
	for (die = 0; die <= topo.max_die_id; ++die) {

		sprintf(path, "/sys/devices/system/cpu/intel_uncore_frequency/package_%02d_die_%02d/current_freq_khz", package, die);

		if (access(path, R_OK) == 0)
			return (snapshot_sysfs_counter(path) / 1000);
	}
	if (!warn_once) {
		warnx("BUG: %s: No %s", __func__, path);
		warn_once = 1;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
void get_apic_id(struct thread_data *t)
{
	unsigned int eax, ebx, ecx, edx;

	if (DO_BIC(BIC_APIC)) {
		eax = ebx = ecx = edx = 0;
		__cpuid(1, eax, ebx, ecx, edx);

		t->apic_id = (ebx >> 24) & 0xff;
	}

	if (!DO_BIC(BIC_X2APIC))
		return;

	if (authentic_amd || hygon_genuine) {
		unsigned int topology_extensions;

		if (max_extended_level < 0x8000001e)
			return;

		eax = ebx = ecx = edx = 0;
		__cpuid(0x80000001, eax, ebx, ecx, edx);
		topology_extensions = ecx & (1 << 22);

		if (topology_extensions == 0)
			return;

		eax = ebx = ecx = edx = 0;
		__cpuid(0x8000001e, eax, ebx, ecx, edx);

		t->x2apic_id = eax;
		return;
	}

	if (!genuine_intel)
		return;

	if (max_level < 0xb)
		return;

	ecx = 0;
	__cpuid(0xb, eax, ebx, ecx, edx);
	t->x2apic_id = edx;

	if (debug && (t->apic_id != (t->x2apic_id & 0xff)))
		fprintf(outf, "cpu%d: BIOS BUG: apic 0x%x x2apic 0x%x\n", t->cpu_id, t->apic_id, t->x2apic_id);
}
/* …（同文件无关代码省略）… */
int get_core_throt_cnt(int cpu, unsigned long long *cnt)
{
	char path[128 + PATH_BYTES];
	unsigned long long tmp;
	FILE *fp;
	int ret;

	sprintf(path, "/sys/devices/system/cpu/cpu%d/thermal_throttle/core_throttle_count", cpu);
	fp = fopen(path, "r");
	if (!fp)
		return -1;
	ret = fscanf(fp, "%lld", &tmp);
	fclose(fp);
	if (ret != 1)
		return -1;
	*cnt = tmp;

	return 0;
}
/* …（同文件无关代码省略）… */
size_t rapl_counter_info_count_perf(const struct rapl_counter_info_t *rci)
{
	size_t ret = 0;

	for (int i = 0; i < NUM_RAPL_COUNTERS; ++i)
		if (rci->source[i] == COUNTER_SOURCE_PERF)
			++ret;

	return ret;
}
/* …（同文件无关代码省略）… */
static size_t cstate_counter_info_count_perf(const struct cstate_counter_info_t *cci)
{
	size_t ret = 0;

	for (int i = 0; i < NUM_CSTATE_COUNTERS; ++i)
		if (cci->source[i] == COUNTER_SOURCE_PERF)
			++ret;

	return ret;
}
/* …（同文件无关代码省略）… */
void write_rapl_counter(struct rapl_counter *rc, struct rapl_counter_info_t *rci, unsigned int idx)
{
	if (rci->source[idx] == COUNTER_SOURCE_NONE)
		return;

	rc->raw_value = rci->data[idx];
	rc->unit = rci->unit[idx];
	rc->scale = rci->scale[idx];
}
/* …（同文件无关代码省略）… */
int get_rapl_counters(int cpu, unsigned int domain, struct core_data *c, struct pkg_data *p)
{
	struct platform_counters *pplat_cnt = p == odd.packages ? &platform_counters_odd : &platform_counters_even;
	unsigned long long perf_data[NUM_RAPL_COUNTERS + 1];
	struct rapl_counter_info_t *rci;

	if (debug >= 2)
		fprintf(stderr, "%s: cpu%d domain%d\n", __func__, cpu, domain);

	assert(rapl_counter_info_perdomain);
	assert(domain < rapl_counter_info_perdomain_size);

	rci = &rapl_counter_info_perdomain[domain];

	/*
	 * If we have any perf counters to read, read them all now, in bulk
	 */
	if (rci->fd_perf != -1) {
		size_t num_perf_counters = rapl_counter_info_count_perf(rci);
		const ssize_t expected_read_size = (num_perf_counters + 1) * sizeof(unsigned long long);
		const ssize_t actual_read_size = read(rci->fd_perf, &perf_data[0], sizeof(perf_data));

		if (actual_read_size != expected_read_size)
			err(-1, "%s: failed to read perf_data (%zu %zu)", __func__, expected_read_size, actual_read_size);
	}

	for (unsigned int i = 0, pi = 1; i < NUM_RAPL_COUNTERS; ++i) {
		switch (rci->source[i]) {
		case COUNTER_SOURCE_NONE:
			rci->data[i] = 0;
			break;

		case COUNTER_SOURCE_PERF:
			assert(pi < ARRAY_SIZE(perf_data));
			assert(rci->fd_perf != -1);

			if (debug >= 2)
				fprintf(stderr, "Reading rapl counter via perf at %u (%llu %e %lf)\n",
					i, perf_data[pi], rci->scale[i], perf_data[pi] * rci->scale[i]);

			rci->data[i] = perf_data[pi];

			++pi;
			break;

		case COUNTER_SOURCE_MSR:
			if (debug >= 2)
				fprintf(stderr, "Reading rapl counter via msr at %u\n", i);

			assert(!no_msr);
			if (rci->flags[i] & RAPL_COUNTER_FLAG_USE_MSR_SUM) {
				if (get_msr_sum(cpu, rci->msr[i], &rci->data[i]))
					return -13 - i;
			} else {
				if (get_msr(cpu, rci->msr[i], &rci->data[i]))
					return -13 - i;
			}

			rci->data[i] &= rci->msr_mask[i];
			if (rci->msr_shift[i] >= 0)
				rci->data[i] >>= abs(rci->msr_shift[i]);
			else
				rci->data[i] <<= abs(rci->msr_shift[i]);

			break;
		}
	}

	BUILD_BUG_ON(NUM_RAPL_COUNTERS != 8);
	write_rapl_counter(&p->energy_pkg, rci, RAPL_RCI_INDEX_ENERGY_PKG);
	write_rapl_counter(&p->energy_cores, rci, RAPL_RCI_INDEX_ENERGY_CORES);
	write_rapl_counter(&p->energy_dram, rci, RAPL_RCI_INDEX_DRAM);
	write_rapl_counter(&p->energy_gfx, rci, RAPL_RCI_INDEX_GFX);
	write_rapl_counter(&p->rapl_pkg_perf_status, rci, RAPL_RCI_INDEX_PKG_PERF_STATUS);
	write_rapl_counter(&p->rapl_dram_perf_status, rci, RAPL_RCI_INDEX_DRAM_PERF_STATUS);
	write_rapl_counter(&c->core_energy, rci, RAPL_RCI_INDEX_CORE_ENERGY);
	write_rapl_counter(&pplat_cnt->energy_psys, rci, RAPL_RCI_INDEX_ENERGY_PLATFORM);

	return 0;
}
/* …（同文件无关代码省略）… */
char *find_sysfs_path_by_id(struct sysfs_path *sp, int id)
{
	while (sp) {
		if (sp->id == id)
			return (sp->path);
		sp = sp->next;
	}
	if (debug)
		warnx("%s: id%d not found", __func__, id);
	return NULL;
}
/* …（同文件无关代码省略）… */
int get_cstate_counters(unsigned int cpu, PER_THREAD_PARAMS)
{
	/*
	 * Overcommit memory a little bit here,
	 * but skip calculating exact sizes for the buffers.
	 */
	unsigned long long perf_data[NUM_CSTATE_COUNTERS];
	unsigned long long perf_data_core[NUM_CSTATE_COUNTERS + 1];
	unsigned long long perf_data_pkg[NUM_CSTATE_COUNTERS + 1];

	struct cstate_counter_info_t *cci;

	if (debug >= 2)
		fprintf(stderr, "%s: cpu%d\n", __func__, cpu);

	assert(ccstate_counter_info);
	assert(cpu <= ccstate_counter_info_size);

	ZERO_ARRAY(perf_data);
	ZERO_ARRAY(perf_data_core);
	ZERO_ARRAY(perf_data_pkg);

	cci = &ccstate_counter_info[cpu];

	/*
	 * If we have any perf counters to read, read them all now, in bulk
	 */
	const size_t num_perf_counters = cstate_counter_info_count_perf(cci);
	ssize_t expected_read_size = num_perf_counters * sizeof(unsigned long long);
	ssize_t actual_read_size_core = 0, actual_read_size_pkg = 0;

	if (cci->fd_perf_core != -1) {
		/* Each descriptor read begins with number of counters read. */
		expected_read_size += sizeof(unsigned long long);

		actual_read_size_core = read(cci->fd_perf_core, &perf_data_core[0], sizeof(perf_data_core));

		if (actual_read_size_core <= 0)
			err(-1, "%s: read perf %s: %ld", __func__, "core", actual_read_size_core);
	}

	if (cci->fd_perf_pkg != -1) {
		/* Each descriptor read begins with number of counters read. */
		expected_read_size += sizeof(unsigned long long);

		actual_read_size_pkg = read(cci->fd_perf_pkg, &perf_data_pkg[0], sizeof(perf_data_pkg));

		if (actual_read_size_pkg <= 0)
			err(-1, "%s: read perf %s: %ld", __func__, "pkg", actual_read_size_pkg);
	}

	const ssize_t actual_read_size_total = actual_read_size_core + actual_read_size_pkg;

	if (actual_read_size_total != expected_read_size)
		err(-1, "%s: failed to read perf_data (%zu %zu)", __func__, expected_read_size, actual_read_size_total);

	/*
	 * Copy ccstate and pcstate data into unified buffer.
	 *
	 * Skip first element from core and pkg buffers.
	 * Kernel puts there how many counters were read.
	 */
	const size_t num_core_counters = perf_data_core[0];
	const size_t num_pkg_counters = perf_data_pkg[0];

	assert(num_perf_counters == num_core_counters + num_pkg_counters);

	/* Copy ccstate perf data */
	memcpy(&perf_data[0], &perf_data_core[1], num_core_counters * sizeof(unsigned long long));

	/* Copy pcstate perf data */
	memcpy(&perf_data[num_core_counters], &perf_data_pkg[1], num_pkg_counters * sizeof(unsigned long long));

	for (unsigned int i = 0, pi = 0; i < NUM_CSTATE_COUNTERS; ++i) {
		switch (cci->source[i]) {
		case COUNTER_SOURCE_NONE:
			break;

		case COUNTER_SOURCE_PERF:
			assert(pi < ARRAY_SIZE(perf_data));
			assert(cci->fd_perf_core != -1 || cci->fd_perf_pkg != -1);

			if (debug >= 2)
				fprintf(stderr, "cstate via %s %u: %llu\n", "perf", i, perf_data[pi]);

			cci->data[i] = perf_data[pi];

			++pi;
			break;

		case COUNTER_SOURCE_MSR:
			assert(!no_msr);
			if (get_msr(cpu, cci->msr[i], &cci->data[i]))
				return -13 - i;

			if (debug >= 2)
				fprintf(stderr, "cstate via %s0x%llx %u: %llu\n", "msr", cci->msr[i], i, cci->data[i]);

			break;
		}
	}

	/*
	 * Helper to write the data only if the source of
	 * the counter for the current cpu is not none.
	 *
	 * Otherwise we would overwrite core data with 0 (default value),
	 * when invoked for the thread sibling.
	 */
#define PERF_COUNTER_WRITE_DATA(out_counter, index) do {	\
	if (cci->source[index] != COUNTER_SOURCE_NONE)		\
		out_counter = cci->data[index];			\
} while (0)

	BUILD_BUG_ON(NUM_CSTATE_COUNTERS != 11);

	PERF_COUNTER_WRITE_DATA(t->c1, CCSTATE_RCI_INDEX_C1_RESIDENCY);
	PERF_COUNTER_WRITE_DATA(c->c3, CCSTATE_RCI_INDEX_C3_RESIDENCY);
	PERF_COUNTER_WRITE_DATA(c->c6, CCSTATE_RCI_INDEX_C6_RESIDENCY);
	PERF_COUNTER_WRITE_DATA(c->c7, CCSTATE_RCI_INDEX_C7_RESIDENCY);

	PERF_COUNTER_WRITE_DATA(p->pc2, PCSTATE_RCI_INDEX_C2_RESIDENCY);
	PERF_COUNTER_WRITE_DATA(p->pc3, PCSTATE_RCI_INDEX_C3_RESIDENCY);
	PERF_COUNTER_WRITE_DATA(p->pc6, PCSTATE_RCI_INDEX_C6_RESIDENCY);
	PERF_COUNTER_WRITE_DATA(p->pc7, PCSTATE_RCI_INDEX_C7_RESIDENCY);
	PERF_COUNTER_WRITE_DATA(p->pc8, PCSTATE_RCI_INDEX_C8_RESIDENCY);
	PERF_COUNTER_WRITE_DATA(p->pc9, PCSTATE_RCI_INDEX_C9_RESIDENCY);
	PERF_COUNTER_WRITE_DATA(p->pc10, PCSTATE_RCI_INDEX_C10_RESIDENCY);

#undef PERF_COUNTER_WRITE_DATA

	return 0;
}
/* …（同文件无关代码省略）… */
size_t msr_counter_info_count_perf(const struct msr_counter_info_t *mci)
{
	size_t ret = 0;

	for (int i = 0; i < NUM_MSR_COUNTERS; ++i)
		if (mci->source[i] == COUNTER_SOURCE_PERF)
			++ret;

	return ret;
}
/* …（同文件无关代码省略）… */
int get_smi_aperf_mperf(unsigned int cpu, struct thread_data *t)
{
	unsigned long long perf_data[NUM_MSR_COUNTERS + 1];

	struct msr_counter_info_t *mci;

	if (debug >= 2)
		fprintf(stderr, "%s: cpu%d\n", __func__, cpu);

	assert(msr_counter_info);
	assert(cpu <= msr_counter_info_size);

	mci = &msr_counter_info[cpu];

	ZERO_ARRAY(perf_data);
	ZERO_ARRAY(mci->data);

	if (mci->fd_perf != -1) {
		const size_t num_perf_counters = msr_counter_info_count_perf(mci);
		const ssize_t expected_read_size = (num_perf_counters + 1) * sizeof(unsigned long long);
		const ssize_t actual_read_size = read(mci->fd_perf, &perf_data[0], sizeof(perf_data));

		if (actual_read_size != expected_read_size)
			err(-1, "%s: failed to read perf_data (%zu %zu)", __func__, expected_read_size, actual_read_size);
	}

	for (unsigned int i = 0, pi = 1; i < NUM_MSR_COUNTERS; ++i) {
		switch (mci->source[i]) {
		case COUNTER_SOURCE_NONE:
			break;

		case COUNTER_SOURCE_PERF:
			assert(pi < ARRAY_SIZE(perf_data));
			assert(mci->fd_perf != -1);

			if (debug >= 2)
				fprintf(stderr, "Reading msr counter via perf at %u: %llu\n", i, perf_data[pi]);

			mci->data[i] = perf_data[pi];

			++pi;
			break;

		case COUNTER_SOURCE_MSR:
			assert(!no_msr);

			if (get_msr(cpu, mci->msr[i], &mci->data[i]))
				return -2 - i;

			mci->data[i] &= mci->msr_mask[i];

			if (debug >= 2)
				fprintf(stderr, "Reading msr counter via msr at %u: %llu\n", i, mci->data[i]);

			break;
		}
	}

	BUILD_BUG_ON(NUM_MSR_COUNTERS != 3);
	t->aperf = mci->data[MSR_RCI_INDEX_APERF];
	t->mperf = mci->data[MSR_RCI_INDEX_MPERF];
	t->smi_count = mci->data[MSR_RCI_INDEX_SMI];

	return 0;
}
/* …（同文件无关代码省略）… */
int perf_counter_info_read_values(struct perf_counter_info *pp, int cpu, unsigned long long *out, size_t out_size)
{
	unsigned int domain;
	unsigned long long value;
	int fd_counter;

	for (size_t i = 0; pp; ++i, pp = pp->next) {
		domain = cpu_to_domain(pp, cpu);
		assert(domain < pp->num_domains);

		fd_counter = pp->fd_perf_per_domain[domain];

		if (fd_counter == -1)
			continue;

		if (read(fd_counter, &value, sizeof(value)) != sizeof(value))
			return 1;

		assert(i < out_size);
		out[i] = value * pp->scale;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
unsigned long pmt_gen_value_mask(unsigned int lsb, unsigned int msb)
{
	unsigned long mask;

	if (msb == 63)
		mask = 0xffffffffffffffff;
	else
		mask = ((1 << (msb + 1)) - 1);

	mask -= (1 << lsb) - 1;

	return mask;
}
/* …（同文件无关代码省略）… */
unsigned long pmt_read_counter(struct pmt_counter *ppmt, unsigned int domain_id)
{
	if (domain_id >= ppmt->num_domains)
		return 0;

	const unsigned long *pmmio = ppmt->domains[domain_id].pcounter;
	const unsigned long value = pmmio ? *pmmio : 0;
	const unsigned long value_mask = pmt_gen_value_mask(ppmt->lsb, ppmt->msb);
	const unsigned long value_shift = ppmt->lsb;

	return (value & value_mask) >> value_shift;
}
/* …（同文件无关代码省略）… */
static inline int get_rapl_domain_id(int cpu)
{
	if (!platform->has_per_core_rapl)
		return cpus[cpu].package_id;

	return GLOBAL_CORE_ID(cpus[cpu].core_id, cpus[cpu].package_id);
}
/* …（同文件无关代码省略）… */
int get_counters(PER_THREAD_PARAMS)
{
	int cpu = t->cpu_id;
	unsigned long long msr;
	struct msr_counter *mp;
	struct pmt_counter *pp;
	int i;
	int status;

	if (cpu_migrate(cpu)) {
		fprintf(outf, "%s: Could not migrate to CPU %d\n", __func__, cpu);
		return -1;
	}

	gettimeofday(&t->tv_begin, (struct timezone *)NULL);

	if (first_counter_read)
		get_apic_id(t);

	t->tsc = rdtsc();	/* we are running on local CPU of interest */

	get_smi_aperf_mperf(cpu, t);

	if (DO_BIC(BIC_LLC_MRPS) || DO_BIC(BIC_LLC_HIT))
		get_perf_llc_stats(cpu, &t->llc);

	if (DO_BIC(BIC_L2_MRPS) || DO_BIC(BIC_L2_HIT))
		get_perf_l2_stats(cpu, &t->l2);

	if (DO_BIC(BIC_IPC))
		if (read(get_instr_count_fd(cpu), &t->instr_count, sizeof(long long)) != sizeof(long long))
			return -4;

	if (DO_BIC(BIC_IRQ))
		t->irq_count = irqs_per_cpu[cpu];
	if (DO_BIC(BIC_NMI))
		t->nmi_count = nmi_per_cpu[cpu];

	get_cstate_counters(cpu, t, c, p);

	for (i = 0, mp = sys.tp; mp; i++, mp = mp->next) {
		if (get_mp(cpu, mp, &t->counter[i], mp->sp->path))
			return -10;
	}

	if (perf_counter_info_read_values(sys.perf_tp, cpu, t->perf_counter, MAX_ADDED_THREAD_COUNTERS))
		return -10;

	for (i = 0, pp = sys.pmt_tp; pp; i++, pp = pp->next)
		t->pmt_counter[i] = pmt_read_counter(pp, t->cpu_id);

	/* collect core counters only for 1st thread in core */
	if (!is_cpu_first_thread_in_core(t, c))
		goto done;

	if (platform->has_per_core_rapl) {
		status = get_rapl_counters(cpu, get_rapl_domain_id(cpu), c, p);
		if (status != 0)
			return status;
	}

	if (DO_BIC(BIC_CPU_c7) && t->is_atom) {
		/*
		 * For Atom CPUs that has core cstate deeper than c6,
		 * MSR_CORE_C6_RESIDENCY returns residency of cc6 and deeper.
		 * Minus CC7 (and deeper cstates) residency to get
		 * accturate cc6 residency.
		 */
		c->c6 -= c->c7;
	}

	if (DO_BIC(BIC_Mod_c6))
		if (get_msr(cpu, MSR_MODULE_C6_RES_MS, &c->mc6_us))
			return -8;

	if (DO_BIC(BIC_CoreTmp)) {
		if (get_msr(cpu, MSR_IA32_THERM_STATUS, &msr))
			return -9;
		c->core_temp_c = tj_max - ((msr >> 16) & 0x7F);
	}

	if (DO_BIC(BIC_CORE_THROT_CNT))
		get_core_throt_cnt(cpu, &c->core_throt_cnt);

	for (i = 0, mp = sys.cp; mp; i++, mp = mp->next) {
		if (get_mp(cpu, mp, &c->counter[i], mp->sp->path))
			return -10;
	}

	if (perf_counter_info_read_values(sys.perf_cp, cpu, c->perf_counter, MAX_ADDED_CORE_COUNTERS))
		return -10;

	for (i = 0, pp = sys.pmt_cp; pp; i++, pp = pp->next)
		c->pmt_counter[i] = pmt_read_counter(pp, cpus[t->cpu_id].core_id);

	/* collect package counters only for 1st core in package */
	if (!is_cpu_first_core_in_package(t, p))
		goto done;

	if (DO_BIC(BIC_Totl_c0)) {
		if (get_msr(cpu, MSR_PKG_WEIGHTED_CORE_C0_RES, &p->pkg_wtd_core_c0))
			return -10;
	}
	if (DO_BIC(BIC_Any_c0)) {
		if (get_msr(cpu, MSR_PKG_ANY_CORE_C0_RES, &p->pkg_any_core_c0))
			return -11;
	}
	if (DO_BIC(BIC_GFX_c0)) {
		if (get_msr(cpu, MSR_PKG_ANY_GFXE_C0_RES, &p->pkg_any_gfxe_c0))
			return -12;
	}
	if (DO_BIC(BIC_CPUGFX)) {
		if (get_msr(cpu, MSR_PKG_BOTH_CORE_GFXE_C0_RES, &p->pkg_both_core_gfxe_c0))
			return -13;
	}

	if (DO_BIC(BIC_CPU_LPI))
		p->cpu_lpi = cpuidle_cur_cpu_lpi_us;
	if (DO_BIC(BIC_SYS_LPI))
		p->sys_lpi = cpuidle_cur_sys_lpi_us;

	if (!platform->has_per_core_rapl) {
		status = get_rapl_counters(cpu, get_rapl_domain_id(cpu), c, p);
		if (status != 0)
			return status;
	}

	if (DO_BIC(BIC_PkgTmp)) {
		if (get_msr(cpu, MSR_IA32_PACKAGE_THERM_STATUS, &msr))
			return -17;
		p->pkg_temp_c = tj_max - ((msr >> 16) & 0x7F);
	}

	if (DO_BIC(BIC_UNCORE_MHZ))
		p->uncore_mhz = get_legacy_uncore_mhz(cpus[t->cpu_id].package_id);

	if (DO_BIC(BIC_GFX_rc6))
		p->gfx_rc6_ms = gfx_info[GFX_rc6].val_ull;

	if (DO_BIC(BIC_GFXMHz))
		p->gfx_mhz = gfx_info[GFX_MHz].val;

	if (DO_BIC(BIC_GFXACTMHz))
		p->gfx_act_mhz = gfx_info[GFX_ACTMHz].val;

	if (DO_BIC(BIC_SAM_mc6))
		p->sam_mc6_ms = gfx_info[SAM_mc6].val_ull;

	if (DO_BIC(BIC_SAMMHz))
		p->sam_mhz = gfx_info[SAM_MHz].val;

	if (DO_BIC(BIC_SAMACTMHz))
		p->sam_act_mhz = gfx_info[SAM_ACTMHz].val;

	for (i = 0, mp = sys.pp; mp; i++, mp = mp->next) {
		char *path = NULL;

		if (mp->msr_num == 0) {
			path = find_sysfs_path_by_id(mp->sp, cpus[t->cpu_id].package_id);
			if (path == NULL) {
				warnx("%s: package_id %d not found", __func__, cpus[t->cpu_id].package_id);
				return -10;
			}
		}
		if (get_mp(cpu, mp, &p->counter[i], path))
			return -10;
	}

	if (perf_counter_info_read_values(sys.perf_pp, cpu, p->perf_counter, MAX_ADDED_PACKAGE_COUNTERS))
		return -10;

	for (i = 0, pp = sys.pmt_pp; pp; i++, pp = pp->next)
		p->pmt_counter[i] = pmt_read_counter(pp, cpus[t->cpu_id].package_id);

done:
	gettimeofday(&t->tv_end, (struct timezone *)NULL);

	return 0;
}
/* …（同文件无关代码省略）… */
		(msr & SNB_C3_AUTO_UNDEMOTE) ? "UNdemote-C3, " : "",
		(msr & SNB_C1_AUTO_UNDEMOTE) ? "UNdemote-C1, " : "",
		(msr & NHM_C3_AUTO_DEMOTE) ? "demote-C3, " : "",
		(msr & NHM_C1_AUTO_DEMOTE) ? "demote-C1, " : "",
		(msr & (1 << 15)) ? "" : "UN", (unsigned int)msr & 0xF, pkg_cstate_limit_strings[pkg_cstate_limit]);

#define AUTOMATIC_CSTATE_CONVERSION		(1UL << 16)
	if (platform->has_cst_auto_convension) {
/* …（同文件无关代码省略）… */
	if (!fd_percpu)
		return;

	for (i = 0; i < topo.max_cpu_num + 1; ++i) {
		if (fd_percpu[i] != 0)
			close(fd_percpu[i]);
	}
/* …（同文件无关代码省略）… */
	if (!fd_instr_count_percpu)
		return;

	for (int i = 0; i < topo.max_cpu_num + 1; ++i) {
		if (fd_instr_count_percpu[i] != 0)
			close(fd_instr_count_percpu[i]);
	}
/* …（同文件无关代码省略）… */
	if (!fd_llc_percpu)
		return;

	for (int i = 0; i < topo.max_cpu_num + 1; ++i) {
		if (fd_llc_percpu[i] != 0)
			close(fd_llc_percpu[i]);
	}
/* …（同文件无关代码省略）… */
void free_fd_l2_percpu(void)
{
	if (!fd_l2_percpu)
		return;

	for (int i = 0; i < topo.max_cpu_num + 1; ++i) {
		if (fd_l2_percpu[i] != 0)
			close(fd_l2_percpu[i]);
	}

	free(fd_l2_percpu);
	fd_l2_percpu = NULL;

	BIC_NOT_PRESENT(BIC_L2_MRPS);
	BIC_NOT_PRESENT(BIC_L2_HIT);
}
/* …（同文件无关代码省略）… */

	CPU_FREE(cpu_present_set);
	cpu_present_set = NULL;
	cpu_present_setsize = 0;

	CPU_FREE(cpu_effective_set);
	cpu_effective_set = NULL;
	cpu_effective_setsize = 0;

	CPU_FREE(cpu_allowed_set);
	cpu_allowed_set = NULL;
	cpu_allowed_setsize = 0;

	CPU_FREE(cpu_affinity_set);
	cpu_affinity_set = NULL;
	cpu_affinity_setsize = 0;

	if (perf_pcore_set) {
		CPU_FREE(perf_pcore_set);
/* …（同文件无关代码省略）… */

static int parse_cpu_str(char *cpu_str, cpu_set_t *cpu_set, int cpu_set_size)
{
	unsigned int start, end;
	char *next = cpu_str;

	while (next && *next) {
/* …（同文件无关代码省略）… */

		start = strtoul(next, &next, 10);

		if (start >= CPU_SUBSET_MAXCPUS)
			return 1;
		CPU_SET_S(start, cpu_set_size, cpu_set);

/* …（同文件无关代码省略）… */
			return 1;

		while (++start <= end) {
			if (start >= CPU_SUBSET_MAXCPUS)
				return 1;
			CPU_SET_S(start, cpu_set_size, cpu_set);
		}
/* …（同文件无关代码省略）… */
int for_all_cpus_2(int (func) (struct thread_data *, struct core_data *,
			       struct pkg_data *, struct thread_data *, struct core_data *,
			       struct pkg_data *), struct thread_data *thread_base,
		   struct core_data *core_base, struct pkg_data *pkg_base,
		   struct thread_data *thread_base2, struct core_data *core_base2, struct pkg_data *pkg_base2)
{
	int cpu, retval;

/* …（同文件无关代码省略）… */
int for_all_proc_cpus(int (func) (int))
{
	FILE *fp;
	int cpu_num;
	int retval;

	fp = fopen_or_die(proc_stat, "r");

	retval = fscanf(fp, "cpu %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d\n");
	if (retval != 0)
		err(1, "%s: failed to parse format", proc_stat);

	while (1) {
		retval = fscanf(fp, "cpu%u %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d\n", &cpu_num);
		if (retval != 1)
			break;

		retval = func(cpu_num);
		if (retval) {
			fclose(fp);
			return (retval);
		}
	}
	fclose(fp);
	return 0;
}
/* …（同文件无关代码省略）… */
#define PATH_EFFECTIVE_CPUS	"/sys/fs/cgroup/cpuset.cpus.effective"
/* …（同文件无关代码省略）… */
static int update_effective_str(bool startup)
{
	FILE *fp;
	char *buf = NULL;
	size_t size = 0;
	int ret;

	if (!cpu_effective_str && !startup)
		return 0;

	fp = fopen(PATH_EFFECTIVE_CPUS, "r");
	if (!fp)
		return 0;

	if (getline(&buf, &size, fp) < 0)
		err(1, "%s: file read failed", PATH_EFFECTIVE_CPUS);

	fclose(fp);

	ret = cpu_effective_str ? strcmp(cpu_effective_str, buf) : 1;
	if (!ret) {
		free(buf);
		return 0;
	}

	free(cpu_effective_str);
	cpu_effective_str = buf;
	return 1;
}
/* …（同文件无关代码省略）… */
static void update_effective_set(bool startup)
{
	update_effective_str(startup);

	if (parse_cpu_str(cpu_effective_str, cpu_effective_set, cpu_effective_setsize))
		err(1, "%s: cpu str malformat %s", PATH_EFFECTIVE_CPUS, cpu_effective_str);
}

/* …（同文件无关代码省略）… */
	fprintf(outf, "turbostat: re-initialized with num_cpus %d, allowed_cpus %d\n", topo.num_cpus, topo.allowed_cpus);
}

void set_max_cpu_num(void)
{
	FILE *filep;
	int current_cpu;
	unsigned long dummy;
	char pathname[64];

	current_cpu = sched_getcpu();
	if (current_cpu < 0)
		err(1, "cannot find calling cpu ID");
	sprintf(pathname, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings", current_cpu);

	filep = fopen_or_die(pathname, "r");
	topo.max_cpu_num = 0;
	while (fscanf(filep, "%lx,", &dummy) == 1)
		topo.max_cpu_num += BITMASK_SIZE;
	fclose(filep);
	topo.max_cpu_num--;	/* 0 based */
}

/*
 * count_cpus()
 * remember the last one seen, it will be the max
/* …（同文件无关代码省略）… */
int count_cpus(int cpu)
{
	UNUSED(cpu);

	topo.num_cpus++;
	return 0;
}

int mark_cpu_present(int cpu)
{
	CPU_SET_S(cpu, cpu_present_setsize, cpu_present_set);
	return 0;
}

int clear_ht_id(int cpu)
{
	int i;

	cpus[cpu].ht_id = -1;
	for (i = 0; i <= MAX_HT_ID; ++i)
		cpus[cpu].ht_sibling_cpu_id[i] = -1;
	return 0;
}
/* …（同文件无关代码省略）… */
int snapshot_proc_interrupts(void)
{
	static FILE *fp;
	int column, retval;

	if (fp == NULL)
		fp = fopen_or_die("/proc/interrupts", "r");
	else
		rewind(fp);

	/* read 1st line of /proc/interrupts to get cpu* name for each column */
	for (column = 0; column < topo.num_cpus; ++column) {
		int cpu_number;

		retval = fscanf(fp, " CPU%d", &cpu_number);
		if (retval != 1)
			break;

		if (cpu_number > topo.max_cpu_num) {
			warn("/proc/interrupts: cpu%d: > %d", cpu_number, topo.max_cpu_num);
			return 1;
		}

		irq_column_2_cpu[column] = cpu_number;
		irqs_per_cpu[cpu_number] = 0;
		nmi_per_cpu[cpu_number] = 0;
	}

	/* read /proc/interrupt count lines and sum up irqs per cpu */
	while (1) {
		int column;
		char buf[64];
		int this_row_is_nmi = 0;

		retval = fscanf(fp, " %s:", buf);	/* irq# "N:" */
		if (retval != 1)
			break;

		if (strncmp(buf, "NMI", strlen("NMI")) == 0)
			this_row_is_nmi = 1;

		/* read the count per cpu */
		for (column = 0; column < topo.num_cpus; ++column) {

			int cpu_number, irq_count;

			retval = fscanf(fp, " %d", &irq_count);

			if (retval != 1)
				break;

			cpu_number = irq_column_2_cpu[column];
			irqs_per_cpu[cpu_number] += irq_count;
			if (this_row_is_nmi)
				nmi_per_cpu[cpu_number] += irq_count;
		}
		while (getc(fp) != '\n') ;	/* flush interrupt description */

	}
	return 0;
}
/* …（同文件无关代码省略）… */
int snapshot_graphics(int idx)
{
	int retval;

	rewind(gfx_info[idx].fp);
	fflush(gfx_info[idx].fp);

	switch (idx) {
	case GFX_rc6:
	case SAM_mc6:
		retval = fscanf(gfx_info[idx].fp, "%lld", &gfx_info[idx].val_ull);
		if (retval != 1)
			err(1, "rc6");
		return 0;
	case GFX_MHz:
	case GFX_ACTMHz:
	case SAM_MHz:
	case SAM_ACTMHz:
		retval = fscanf(gfx_info[idx].fp, "%d", &gfx_info[idx].val);
		if (retval != 1)
			err(1, "MHz");
		return 0;
	default:
		return -EINVAL;
	}
}
/* …（同文件无关代码省略）… */
int snapshot_cpu_lpi_us(void)
{
	FILE *fp;
	int retval;

	fp = fopen_or_die("/sys/devices/system/cpu/cpuidle/low_power_idle_cpu_residency_us", "r");

	retval = fscanf(fp, "%lld", &cpuidle_cur_cpu_lpi_us);
	if (retval != 1) {
		fprintf(stderr, "Disabling Low Power Idle CPU output\n");
		BIC_NOT_PRESENT(BIC_CPU_LPI);
		fclose(fp);
		return -1;
	}

	fclose(fp);

	return 0;
}
/* …（同文件无关代码省略）… */
int snapshot_sys_lpi_us(void)
{
	FILE *fp;
	int retval;

	fp = fopen_or_die(sys_lpi_file, "r");

	retval = fscanf(fp, "%lld", &cpuidle_cur_sys_lpi_us);
	if (retval != 1) {
		fprintf(stderr, "Disabling Low Power Idle System output\n");
		BIC_NOT_PRESENT(BIC_SYS_LPI);
		fclose(fp);
		return -1;
	}
	fclose(fp);

	return 0;
}
/* …（同文件无关代码省略）… */
int snapshot_proc_sysfs_files(void)
{
	gettimeofday(&procsysfs_tv_begin, (struct timezone *)NULL);

	if (DO_BIC(BIC_IRQ) || DO_BIC(BIC_NMI))
		if (snapshot_proc_interrupts())
			return 1;

	if (DO_BIC(BIC_GFX_rc6))
		snapshot_graphics(GFX_rc6);

	if (DO_BIC(BIC_GFXMHz))
		snapshot_graphics(GFX_MHz);

	if (DO_BIC(BIC_GFXACTMHz))
		snapshot_graphics(GFX_ACTMHz);

	if (DO_BIC(BIC_SAM_mc6))
		snapshot_graphics(SAM_mc6);

	if (DO_BIC(BIC_SAMMHz))
		snapshot_graphics(SAM_MHz);

	if (DO_BIC(BIC_SAMACTMHz))
		snapshot_graphics(SAM_ACTMHz);

	if (DO_BIC(BIC_CPU_LPI))
		snapshot_cpu_lpi_us();

	if (DO_BIC(BIC_SYS_LPI))
		snapshot_sys_lpi_us();

	return 0;
}
/* …（同文件无关代码省略）… */

int get_msr_sum(int cpu, off_t offset, unsigned long long *msr)
{
	int ret, idx;
	unsigned long long msr_cur, msr_last;

	assert(!no_msr);
/* …（同文件无关代码省略）… */
	if (idx < 0)
		return idx;
	/* get_msr_sum() = sum + (get_msr() - last) */
	ret = get_msr(cpu, offset, &msr_cur);
	if (ret)
		return ret;
	msr_last = per_cpu_msr_sum[cpu].entries[idx].last;
	DELTA_WRAP32(msr_cur, msr_last);
	*msr = msr_last + per_cpu_msr_sum[cpu].entries[idx].sum;
/* …（同文件无关代码省略）… */
/* Timer callback, update the sum of MSRs periodically. */
static int update_msr_sum(PER_THREAD_PARAMS)
{
	int i, ret;
	int cpu = t->cpu_id;

	UNUSED(c);
/* …（同文件无关代码省略）… */
		offset = idx_to_offset(i);
		if (offset < 0)
			continue;
		ret = get_msr(cpu, offset, &msr_cur);
		if (ret) {
			fprintf(outf, "Can not update msr(0x%llx)\n", (unsigned long long)offset);
			continue;
		}

		msr_last = per_cpu_msr_sum[cpu].entries[i].last;
		per_cpu_msr_sum[cpu].entries[i].last = msr_cur & 0xffffffff;
/* …（同文件无关代码省略）… */
static void msr_record_handler(union sigval v)
{
	UNUSED(v);

	for_all_cpus(update_msr_sum, EVEN_COUNTERS);
}
/* …（同文件无关代码省略）… */
void msr_sum_record(void)
{
	struct itimerspec its;
	struct sigevent sev;

	per_cpu_msr_sum = calloc(topo.max_cpu_num + 1, sizeof(struct msr_sum_array));
	if (!per_cpu_msr_sum) {
		fprintf(outf, "Can not allocate memory for long time MSR.\n");
		return;
	}
	/*
	 * Signal handler might be restricted, so use thread notifier instead.
	 */
	memset(&sev, 0, sizeof(struct sigevent));
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = msr_record_handler;

	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
		fprintf(outf, "Can not create timer.\n");
		goto release_msr;
	}

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 1;
	/*
	 * A wraparound time has been calculated early.
	 * Some sources state that the peak power for a
	 * microprocessor is usually 1.5 times the TDP rating,
	 * use 2 * TDP for safety.
	 */
	its.it_interval.tv_sec = rapl_joule_counter_range / 2;
	its.it_interval.tv_nsec = 0;

	if (timer_settime(timerid, 0, &its, NULL) == -1) {
		fprintf(outf, "Can not set timer.\n");
		goto release_timer;
	}
	return;

release_timer:
	timer_delete(timerid);
release_msr:
	free(per_cpu_msr_sum);
	per_cpu_msr_sum = NULL;
}
/* …（同文件无关代码省略）… */
	}

	if (failed) {
		warnx("Failed to access %s. Some of the counters may not be available\n"
		      "\tRun as root to enable them or use %s to disable the access explicitly", pathname, "--no-msr");
		no_msr = 1;
	}
}
/* …（同文件无关代码省略）… */
static int read_sysfs_int(char *path)
{
	FILE *input;
	int retval = -1;

	input = fopen(path, "r");
	if (input == NULL) {
		if (debug)
			fprintf(outf, "NSFOD %s\n", path);
		return (-1);
	}
	if (fscanf(input, "%d", &retval) != 1)
		err(1, "%s: failed to read int from file", path);
	fclose(input);

	return (retval);
}

static void dump_sysfs_file(char *path)
{
	FILE *input;
/* …（同文件无关代码省略）… */
		if (quiet)
			continue;

		sprintf(path, "%s/min_freq_khz", path_base);
		k = read_sysfs_int(path);
		sprintf(path, "%s/max_freq_khz", path_base);
		l = read_sysfs_int(path);
		fprintf(outf, "Uncore Frequency package%d domain%d cluster%d: %d - %d MHz ", unc_pkg_id, domain_id, cluster_id, k / 1000, l / 1000);

		sprintf(path, "%s/initial_min_freq_khz", path_base);
		k = read_sysfs_int(path);
/* …（同文件无关代码省略）… */
		return -1;
	}

	if (get_msr(cpu, MSR_PM_ENABLE, &msr))
		return 0;

	fprintf(outf, "cpu%d: MSR_PM_ENABLE: 0x%08llx (%sHWP)\n", cpu, msr, (msr & (1 << 0)) ? "" : "No-");

	/* MSR_PM_ENABLE[1] == 1 if HWP is enabled and MSRs visible */
	if ((msr & (1 << 0)) == 0)
		return 0;

	if (get_msr(cpu, MSR_HWP_CAPABILITIES, &msr))
		return 0;

	fprintf(outf, "cpu%d: MSR_HWP_CAPABILITIES: 0x%08llx "
		"(high %d guar %d eff %d low %d)\n",
		cpu, msr,
		(unsigned int)HWP_HIGHEST_PERF(msr),
		(unsigned int)HWP_GUARANTEED_PERF(msr), (unsigned int)HWP_MOSTEFFICIENT_PERF(msr), (unsigned int)HWP_LOWEST_PERF(msr));

	if (get_msr(cpu, MSR_HWP_REQUEST, &msr))
		return 0;

	fprintf(outf, "cpu%d: MSR_HWP_REQUEST: 0x%08llx "
		"(min %d max %d des %d epp 0x%x window 0x%x pkg 0x%x)\n",
		cpu, msr,
		(unsigned int)(((msr) >> 0) & 0xff),
		(unsigned int)(((msr) >> 8) & 0xff),
		(unsigned int)(((msr) >> 16) & 0xff),
		(unsigned int)(((msr) >> 24) & 0xff), (unsigned int)(((msr) >> 32) & 0xff3), (unsigned int)(((msr) >> 42) & 0x1));

	if (has_hwp_pkg) {
		if (get_msr(cpu, MSR_HWP_REQUEST_PKG, &msr))
			return 0;

		fprintf(outf, "cpu%d: MSR_HWP_REQUEST_PKG: 0x%08llx "
			"(min %d max %d des %d epp 0x%x window 0x%x)\n",
			cpu, msr,
			(unsigned int)(((msr) >> 0) & 0xff),
			(unsigned int)(((msr) >> 8) & 0xff),
			(unsigned int)(((msr) >> 16) & 0xff), (unsigned int)(((msr) >> 24) & 0xff), (unsigned int)(((msr) >> 32) & 0xff3));
	}
	if (has_hwp_notify) {
		if (get_msr(cpu, MSR_HWP_INTERRUPT, &msr))
			return 0;

		fprintf(outf, "cpu%d: MSR_HWP_INTERRUPT: 0x%08llx "
			"(%s_Guaranteed_Perf_Change, %s_Excursion_Min)\n", cpu, msr, ((msr) & 0x1) ? "EN" : "Dis", ((msr) & 0x2) ? "EN" : "Dis");
	}
	if (get_msr(cpu, MSR_HWP_STATUS, &msr))
		return 0;

	fprintf(outf, "cpu%d: MSR_HWP_STATUS: 0x%08llx "
		"(%sGuaranteed_Perf_Change, %sExcursion_Min)\n", cpu, msr, ((msr) & 0x1) ? "" : "No-", ((msr) & 0x4) ? "" : "No-");

	return 0;
}
/* …（同文件无关代码省略）… */
			(msr & 1 << 8) ? "Amps, " : "",
			(msr & 1 << 6) ? "VR-Therm, " : "",
			(msr & 1 << 5) ? "Auto-HWP, " : "",
			(msr & 1 << 4) ? "Graphics, " : "",
			(msr & 1 << 2) ? "bit2, " : "", (msr & 1 << 1) ? "ThermStatus, " : "", (msr & 1 << 0) ? "PROCHOT, " : "");
		fprintf(outf, " (Logged: %s%s%s%s%s%s%s%s%s%s%s%s%s%s)\n",
			(msr & 1 << 31) ? "bit31, " : "",
			(msr & 1 << 30) ? "bit30, " : "",
/* …（同文件无关代码省略）… */
			(msr & 1 << 24) ? "Amps, " : "",
			(msr & 1 << 22) ? "VR-Therm, " : "",
			(msr & 1 << 21) ? "Auto-HWP, " : "",
			(msr & 1 << 20) ? "Graphics, " : "",
			(msr & 1 << 18) ? "bit18, " : "", (msr & 1 << 17) ? "ThermStatus, " : "", (msr & 1 << 16) ? "PROCHOT, " : "");

	}
	if (platform->plr_msrs & PLR_GFX) {
/* …（同文件无关代码省略）… */
			(msr & 1 << 1) ? "ThermStatus, " : "",
			(msr & 1 << 4) ? "Graphics, " : "",
			(msr & 1 << 6) ? "VR-Therm, " : "",
			(msr & 1 << 8) ? "Amps, " : "",
			(msr & 1 << 9) ? "GFXPwr, " : "", (msr & 1 << 10) ? "PkgPwrL1, " : "", (msr & 1 << 11) ? "PkgPwrL2, " : "");
		fprintf(outf, " (Logged: %s%s%s%s%s%s%s%s)\n",
			(msr & 1 << 16) ? "PROCHOT, " : "",
			(msr & 1 << 17) ? "ThermStatus, " : "",
			(msr & 1 << 20) ? "Graphics, " : "",
			(msr & 1 << 22) ? "VR-Therm, " : "",
			(msr & 1 << 24) ? "Amps, " : "",
			(msr & 1 << 25) ? "GFXPwr, " : "", (msr & 1 << 26) ? "PkgPwrL1, " : "", (msr & 1 << 27) ? "PkgPwrL2, " : "");
	}
	if (platform->plr_msrs & PLR_RING) {
		get_msr(cpu, MSR_RING_PERF_LIMIT_REASONS, &msr);
		fprintf(outf, "cpu%d: MSR_RING_PERF_LIMIT_REASONS, 0x%08llx", cpu, msr);
		fprintf(outf, " (Active: %s%s%s%s%s%s)",
			(msr & 1 << 0) ? "PROCHOT, " : "",
			(msr & 1 << 1) ? "ThermStatus, " : "",
			(msr & 1 << 6) ? "VR-Therm, " : "",
			(msr & 1 << 8) ? "Amps, " : "", (msr & 1 << 10) ? "PkgPwrL1, " : "", (msr & 1 << 11) ? "PkgPwrL2, " : "");
		fprintf(outf, " (Logged: %s%s%s%s%s%s)\n",
			(msr & 1 << 16) ? "PROCHOT, " : "",
			(msr & 1 << 17) ? "ThermStatus, " : "",
			(msr & 1 << 22) ? "VR-Therm, " : "",
			(msr & 1 << 24) ? "Amps, " : "", (msr & 1 << 26) ? "PkgPwrL1, " : "", (msr & 1 << 27) ? "PkgPwrL2, " : "");
	}
	return 0;
}
/* …（同文件无关代码省略）… */
#define	RAPL_POWER_GRANULARITY	0x7FFF	/* 15 bit power granularity */
#define	RAPL_TIME_GRANULARITY	0x3F	/* 6 bit time granularity */
/* …（同文件无关代码省略）… */
double get_quirk_tdp(void)
{
	if (platform->rapl_quirk_tdp)
		return platform->rapl_quirk_tdp;

	return 135.0;
}
/* …（同文件无关代码省略）… */
{
	unsigned long long msr;

	if (valid_rapl_msrs & RAPL_PKG_POWER_INFO)
		if (!get_msr(master_cpu, MSR_PKG_POWER_INFO, &msr))
			return ((msr >> 0) & RAPL_POWER_GRANULARITY) * rapl_power_units;
	return get_quirk_tdp();
}

/* …（同文件无关代码省略）… */
		CLR_BIC(BIC_RAM__, &bic_enabled);

	/* units on package 0, verify later other packages match */
	if (get_msr(master_cpu, MSR_RAPL_POWER_UNIT, &msr))
		return;

	rapl_power_units = 1.0 / (1 << (msr & 0xF));
	if (platform->has_rapl_divisor)
/* …（同文件无关代码省略）… */
	if (!valid_rapl_msrs || no_msr)
		return;

	if (get_msr(master_cpu, MSR_RAPL_PWR_UNIT, &msr))
		return;

	rapl_time_units = ldexp(1.0, -(msr >> 16 & 0xf));
	rapl_energy_units = ldexp(1.0, -(msr >> 8 & 0x1f));
	rapl_power_units = ldexp(1.0, -(msr & 0xf));
/* …（同文件无关代码省略）… */
void print_power_limit_msr(int cpu, unsigned long long msr, char *label)
{
	fprintf(outf, "cpu%d: %s: %sabled (%0.3f Watts, %f sec, clamp %sabled)\n",
		cpu, label,
		((msr >> 15) & 1) ? "EN" : "DIS",
		((msr >> 0) & 0x7FFF) * rapl_power_units,
		(1.0 + (((msr >> 22) & 0x3) / 4.0)) * (1 << ((msr >> 17) & 0x1F)) * rapl_time_units, (((msr >> 16) & 1) ? "EN" : "DIS"));

	return;
}
/* …（同文件无关代码省略）… */
#define PATH_RAPL_SYSFS	"/sys/class/powercap"
/* …（同文件无关代码省略）… */

		snprintf(path, PATH_MAX, "%s/%s", PATH_RAPL_SYSFS, entry->d_name);
		if ((cdir = opendir(path)) == NULL) {
			perror("opendir() error");
			return 1;
		}

/* …（同文件无关代码省略）… */

	if (valid_rapl_msrs & RAPL_AMD_F17H) {
		msr_name = "MSR_RAPL_PWR_UNIT";
		if (get_msr(cpu, MSR_RAPL_PWR_UNIT, &msr))
			return -1;
	} else {
		msr_name = "MSR_RAPL_POWER_UNIT";
		if (get_msr(cpu, MSR_RAPL_POWER_UNIT, &msr))
			return -1;
	}

	fprintf(outf, "cpu%d: %s: 0x%08llx (%f Watts, %f Joules, %f sec.)\n", cpu, msr_name, msr, rapl_power_units, rapl_energy_units, rapl_time_units);

	if (valid_rapl_msrs & RAPL_PKG_POWER_INFO) {

		if (get_msr(cpu, MSR_PKG_POWER_INFO, &msr))
			return -5;

		fprintf(outf, "cpu%d: MSR_PKG_POWER_INFO: 0x%08llx (%.0f W TDP, RAPL %.0f - %.0f W, %f sec.)\n",
			cpu, msr,
			((msr >> 0) & RAPL_POWER_GRANULARITY) * rapl_power_units,
/* …（同文件无关代码省略）… */
	}
	if (valid_rapl_msrs & RAPL_PKG) {

		if (get_msr(cpu, MSR_PKG_POWER_LIMIT, &msr))
			return -9;

		fprintf(outf, "cpu%d: MSR_PKG_POWER_LIMIT: 0x%08llx (%slocked)\n", cpu, msr, (msr >> 63) & 1 ? "" : "UN");

		print_power_limit_msr(cpu, msr, "PKG Limit #1");
		fprintf(outf, "cpu%d: PKG Limit #2: %sabled (%0.3f Watts, %f* sec, clamp %sabled)\n",
			cpu,
			((msr >> 47) & 1) ? "EN" : "DIS",
			((msr >> 32) & 0x7FFF) * rapl_power_units,
			(1.0 + (((msr >> 54) & 0x3) / 4.0)) * (1 << ((msr >> 49) & 0x1F)) * rapl_time_units, ((msr >> 48) & 1) ? "EN" : "DIS");

		if (get_msr(cpu, MSR_VR_CURRENT_CONFIG, &msr))
			return -9;

		fprintf(outf, "cpu%d: MSR_VR_CURRENT_CONFIG: 0x%08llx\n", cpu, msr);
		fprintf(outf, "cpu%d: PKG Limit #4: %f Watts (%slocked)\n", cpu, ((msr >> 0) & 0x1FFF) * rapl_power_units, (msr >> 31) & 1 ? "" : "UN");
	}

	if (valid_rapl_msrs & RAPL_DRAM_POWER_INFO) {
		if (get_msr(cpu, MSR_DRAM_POWER_INFO, &msr))
			return -6;

		fprintf(outf, "cpu%d: MSR_DRAM_POWER_INFO,: 0x%08llx (%.0f W TDP, RAPL %.0f - %.0f W, %f sec.)\n",
			cpu, msr,
			((msr >> 0) & RAPL_POWER_GRANULARITY) * rapl_power_units,
			((msr >> 16) & RAPL_POWER_GRANULARITY) * rapl_power_units,
			((msr >> 32) & RAPL_POWER_GRANULARITY) * rapl_power_units, ((msr >> 48) & RAPL_TIME_GRANULARITY) * rapl_time_units);
	}
	if (valid_rapl_msrs & RAPL_DRAM) {
		if (get_msr(cpu, MSR_DRAM_POWER_LIMIT, &msr))
			return -9;
		fprintf(outf, "cpu%d: MSR_DRAM_POWER_LIMIT: 0x%08llx (%slocked)\n", cpu, msr, (msr >> 31) & 1 ? "" : "UN");

		print_power_limit_msr(cpu, msr, "DRAM Limit");
	}
	if (valid_rapl_msrs & RAPL_CORE_POLICY) {
		if (get_msr(cpu, MSR_PP0_POLICY, &msr))
			return -7;

		fprintf(outf, "cpu%d: MSR_PP0_POLICY: %lld\n", cpu, msr & 0xF);
	}
	if (valid_rapl_msrs & RAPL_CORE_POWER_LIMIT) {
		if (get_msr(cpu, MSR_PP0_POWER_LIMIT, &msr))
			return -9;
		fprintf(outf, "cpu%d: MSR_PP0_POWER_LIMIT: 0x%08llx (%slocked)\n", cpu, msr, (msr >> 31) & 1 ? "" : "UN");
		print_power_limit_msr(cpu, msr, "Cores Limit");
	}
	if (valid_rapl_msrs & RAPL_GFX) {
		if (get_msr(cpu, MSR_PP1_POLICY, &msr))
			return -8;

		fprintf(outf, "cpu%d: MSR_PP1_POLICY: %lld\n", cpu, msr & 0xF);

		if (get_msr(cpu, MSR_PP1_POWER_LIMIT, &msr))
			return -9;
		fprintf(outf, "cpu%d: MSR_PP1_POWER_LIMIT: 0x%08llx (%slocked)\n", cpu, msr, (msr >> 31) & 1 ? "" : "UN");
		print_power_limit_msr(cpu, msr, "GFX Limit");
	}
/* …（同文件无关代码省略）… */
 */
void probe_rapl_msrs(void)
{
	int ret;
	off_t offset;
	unsigned long long msr_value;

/* …（同文件无关代码省略）… */
	if (offset < 0)
		return;

	ret = get_msr(master_cpu, offset, &msr_value);
	if (ret) {
		if (debug)
			fprintf(outf, "Can not read RAPL_PKG_ENERGY MSR(0x%llx)\n", (unsigned long long)offset);
		return;
	}
	if (msr_value == 0) {
		if (debug)
			fprintf(outf, "RAPL_PKG_ENERGY MSR(0x%llx) == ZERO: disabling all RAPL MSRs\n", (unsigned long long)offset);
/* …（同文件无关代码省略）… */
	if (!platform->has_nhm_msrs || no_msr)
		goto guess;

	if (get_msr(master_cpu, MSR_IA32_TEMPERATURE_TARGET, &msr))
		goto guess;

	tcc_default = (msr >> 16) & 0xFF;

	if (!quiet) {
		int bits = platform->tcc_offset_bits;
		unsigned long long enabled = 0;

		if (bits && !get_msr(master_cpu, MSR_PLATFORM_INFO, &enabled))
			enabled = (enabled >> 30) & 1;

		if (bits && enabled) {
			tcc_offset = (msr >> 24) & GENMASK(bits - 1, 0);
			fprintf(outf, "cpu%d: MSR_IA32_TEMPERATURE_TARGET: 0x%08llx (%d C) (%d default - %d offset)\n",
				cpu, msr, tcc_default - tcc_offset, tcc_default, tcc_offset);
		} else {
			fprintf(outf, "cpu%d: MSR_IA32_TEMPERATURE_TARGET: 0x%08llx (%d C)\n", cpu, msr, tcc_default);
		}
/* …（同文件无关代码省略）… */
	}

	if (do_ptm && is_cpu_first_core_in_package(t, p)) {
		if (get_msr(cpu, MSR_IA32_PACKAGE_THERM_STATUS, &msr))
			return 0;

		dts = (msr >> 16) & 0x7F;
		fprintf(outf, "cpu%d: MSR_IA32_PACKAGE_THERM_STATUS: 0x%08llx (%d C)\n", cpu, msr, tj_max - dts);

		if (get_msr(cpu, MSR_IA32_PACKAGE_THERM_INTERRUPT, &msr))
			return 0;

		dts = (msr >> 16) & 0x7F;
		dts2 = (msr >> 8) & 0x7F;
		fprintf(outf, "cpu%d: MSR_IA32_PACKAGE_THERM_INTERRUPT: 0x%08llx (%d C, %d C)\n", cpu, msr, tj_max - dts, tj_max - dts2);
/* …（同文件无关代码省略）… */
	if (do_dts && debug) {
		unsigned int resolution;

		if (get_msr(cpu, MSR_IA32_THERM_STATUS, &msr))
			return 0;

		dts = (msr >> 16) & 0x7F;
		resolution = (msr >> 27) & 0xF;
		fprintf(outf, "cpu%d: MSR_IA32_THERM_STATUS: 0x%08llx (%d C +/- %d)\n", cpu, msr, tj_max - dts, resolution);

		if (get_msr(cpu, MSR_IA32_THERM_INTERRUPT, &msr))
			return 0;

		dts = (msr >> 16) & 0x7F;
		dts2 = (msr >> 8) & 0x7F;
		fprintf(outf, "cpu%d: MSR_IA32_THERM_INTERRUPT: 0x%08llx (%d C, %d C)\n", cpu, msr, tj_max - dts, tj_max - dts2);
/* …（同文件无关代码省略）… */
void decode_feature_control_msr(void)
{
	unsigned long long msr;

	if (no_msr)
		return;

	if (quiet)
		return;

	if (!get_msr(master_cpu, MSR_IA32_FEAT_CTL, &msr))
		fprintf(outf, "cpu%d: MSR_IA32_FEATURE_CONTROL: 0x%08llx (%sLocked %s)\n",
			master_cpu, msr, msr & FEAT_CTL_LOCKED ? "" : "UN-", msr & (1 << 18) ? "SGX" : "");
}

void decode_misc_enable_msr(void)
{
	unsigned long long msr;

	if (no_msr)
		return;

	if (!genuine_intel)
		return;

	if (!get_msr(master_cpu, MSR_IA32_MISC_ENABLE, &msr))
		fprintf(outf, "cpu%d: MSR_IA32_MISC_ENABLE: 0x%08llx (%sTCC %sEIST %sMWAIT %sPREFETCH %sTURBO)\n",
			master_cpu, msr,
			msr & MSR_IA32_MISC_ENABLE_TM1 ? "" : "No-",
			msr & MSR_IA32_MISC_ENABLE_ENHANCED_SPEEDSTEP ? "" : "No-",
			msr & MSR_IA32_MISC_ENABLE_MWAIT ? "" : "No-",
			msr & MSR_IA32_MISC_ENABLE_PREFETCH_DISABLE ? "No-" : "", msr & MSR_IA32_MISC_ENABLE_TURBO_DISABLE ? "No-" : "");
}

void decode_misc_feature_control(void)
{
	unsigned long long msr;

	if (no_msr)
		return;

	if (!platform->has_msr_misc_feature_control)
		return;

	if (!get_msr(master_cpu, MSR_MISC_FEATURE_CONTROL, &msr))
		fprintf(outf,
			"cpu%d: MSR_MISC_FEATURE_CONTROL: 0x%08llx (%sL2-Prefetch %sL2-Prefetch-pair %sL1-Prefetch %sL1-IP-Prefetch)\n",
			master_cpu, msr, msr & (0 << 0) ? "No-" : "", msr & (1 << 0) ? "No-" : "", msr & (2 << 0) ? "No-" : "", msr & (3 << 0) ? "No-" : "");
}

/*
/* …（同文件无关代码省略）… */
	if (!platform->has_msr_misc_pwr_mgmt)
		return;

	if (!get_msr(master_cpu, MSR_MISC_PWR_MGMT, &msr))
		fprintf(outf, "cpu%d: MSR_MISC_PWR_MGMT: 0x%08llx (%sable-EIST_Coordination %sable-EPB %sable-OOB)\n",
			master_cpu, msr, msr & (1 << 0) ? "DIS" : "EN", msr & (1 << 1) ? "EN" : "DIS", msr & (1 << 8) ? "EN" : "DIS");
}

/*
/* …（同文件无关代码省略）… */
	if (!platform->has_msr_c6_demotion_policy_config)
		return;

	if (!get_msr(master_cpu, MSR_CC6_DEMOTION_POLICY_CONFIG, &msr))
		fprintf(outf, "cpu%d: MSR_CC6_DEMOTION_POLICY_CONFIG: 0x%08llx (%sable-CC6-Demotion)\n", master_cpu, msr, msr & (1 << 0) ? "EN" : "DIS");

	if (!get_msr(master_cpu, MSR_MC6_DEMOTION_POLICY_CONFIG, &msr))
		fprintf(outf, "cpu%d: MSR_MC6_DEMOTION_POLICY_CONFIG: 0x%08llx (%sable-MC6-Demotion)\n", master_cpu, msr, msr & (1 << 0) ? "EN" : "DIS");
}

void print_dev_latency(void)
{
	char *path = "/dev/cpu_dma_latency";
	int fd;
	int value;
	int retval;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		if (debug)
			warnx("Read %s failed", path);
		return;
	}

	retval = read(fd, (void *)&value, sizeof(int));
	if (retval != sizeof(int)) {
		warn("read failed %s", path);
		close(fd);
		return;
	}
	fprintf(outf, "/dev/cpu_dma_latency: %d usec (%s)\n", value, value == 2000000000 ? "default" : "constrained");

	close(fd);
}
/* …（同文件无关代码省略）… */

	if (fd == -1)
		warnx("Failed to access %s. Some of the counters may not be available\n"
		      "\tRun as root to enable them or use %s to disable the access explicitly", "perf instructions retired counter",
		      "'--hide IPC' or '--no-perf'");

	return (fd != -1);
}
/* …（同文件无关代码省略）… */
	return ret;
}

int initialize_cpu_set_from_sysfs(cpu_set_t *cpu_set, const char *sysfs_path,
				  const char *sysfs_file)
{
	FILE *fp;
	char *cpuset_buf = NULL;
/* …（同文件无关代码省略）… */
		warn("read %s", sysfs_path);
		goto out;
	}
	if (parse_cpu_str(cpuset_buf, cpu_set, cpu_possible_setsize)) {
		warnx("%s: cpu str malformat %s\n", sysfs_path, cpuset_buf);
		goto out;
	}
/* …（同文件无关代码省略）… */
void print_cpu_set(char *s, cpu_set_t *set)
{
	int i;

	assert(MAX_BIC < CPU_SETSIZE);

	printf("%s:", s);

	for (i = 0; i <= topo.max_cpu_num; ++i)
		if (CPU_ISSET(i, set))
			printf(" %d", i);
	putchar('\n');
}
/* …（同文件无关代码省略）… */
		perf_pcore_set = CPU_ALLOC((topo.max_cpu_num + 1));
		if (perf_pcore_set == NULL)
			err(3, "CPU_ALLOC");
		CPU_ZERO_S(cpu_possible_setsize, perf_pcore_set);
		initialize_cpu_set_from_sysfs(perf_pcore_set, perf_cpu_pcore_path, "cpus");
		if (debug)
			print_cpu_set("perf pcores", perf_pcore_set);
/* …（同文件无关代码省略）… */
		perf_ecore_set = CPU_ALLOC((topo.max_cpu_num + 1));
		if (perf_ecore_set == NULL)
			err(3, "CPU_ALLOC");
		CPU_ZERO_S(cpu_possible_setsize, perf_ecore_set);
		initialize_cpu_set_from_sysfs(perf_ecore_set, perf_cpu_ecore_path, "cpus");
		if (debug)
			print_cpu_set("perf ecores", perf_ecore_set);
/* …（同文件无关代码省略）… */
		perf_lcore_set = CPU_ALLOC((topo.max_cpu_num + 1));
		if (perf_lcore_set == NULL)
			err(3, "CPU_ALLOC");
		CPU_ZERO_S(cpu_possible_setsize, perf_lcore_set);
		initialize_cpu_set_from_sysfs(perf_lcore_set, perf_cpu_lcore_path, "cpus");
		if (debug)
			print_cpu_set("perf lcores", perf_lcore_set);
/* …（同文件无关代码省略）… */

		memset(domain_visited, 0, num_domains * sizeof(*domain_visited));

		for (int cpu = 0; cpu < topo.max_cpu_num + 1; ++cpu) {

			if (cpu_is_not_allowed(cpu))
				continue;
/* …（同文件无关代码省略）… */
void dump_word_chars(unsigned int word)
{
	int i;

	for (i = 0; i < 4; ++i)
		fprintf(outf, "%c", (word >> (i * 8)) & 0xFF);
}
/* …（同文件无关代码省略）… */
void dump_cpuid_hypervisor(void)
{
	unsigned int ebx = 0;
	unsigned int ecx = 0;
	unsigned int edx = 0;

	__cpuid(0x40000000, max_extended_level, ebx, ecx, edx);

	fprintf(outf, "Hypervisor: ");
	dump_word_chars(ebx);
	dump_word_chars(ecx);
	dump_word_chars(edx);
	fprintf(outf, "\n");
}

void process_cpuid()
{
	unsigned int eax, ebx, ecx, edx;
/* …（同文件无关代码省略）… */
	cpuid_has_hv = ecx_flags & (1 << 31);

	if (!no_msr) {
		if (get_msr(sched_getcpu(), MSR_IA32_UCODE_REV, &ucode_patch)) {
			warnx("get_msr(UCODE)");
		} else {
			ucode_patch_valid = true;
			if (!authentic_amd && !hygon_genuine)
				ucode_patch >>= 32;
		}
	}

	/*
/* …（同文件无关代码省略）… */
			ecx_flags & (1 << 8) ? "" : "No-",
			cpuid_has_hv ? "" : "No-",
			edx_flags & (1 << 4) ? "" : "No-",
			edx_flags & (1 << 5) ? "" : "No-",
			edx_flags & (1 << 22) ? "" : "No-", edx_flags & (1 << 28) ? "" : "No-", edx_flags & (1 << 29) ? "" : "No-");
	}
	if (!quiet && cpuid_has_hv)
		dump_cpuid_hypervisor();
/* …（同文件无关代码省略）… */
			has_turbo ? "" : "No-",
			do_dts ? "" : "No-",
			do_ptm ? "" : "No-",
			has_hwp ? "" : "No-",
			has_hwp_notify ? "" : "No-",
			has_hwp_activity_window ? "" : "No-", has_hwp_epp ? "" : "No-", has_hwp_pkg ? "" : "No-", has_epb ? "" : "No-");

	if (!quiet)
		decode_misc_enable_msr();

	if (max_level >= 0x7) {
		int has_sgx;

		ecx = 0;

		__cpuid_count(0x7, 0, eax, ebx, ecx, edx);

		has_sgx = ebx & (1 << 2);

		is_hybrid = !!(edx & (1 << 15));

		if (!quiet)
			fprintf(outf, "CPUID(7): %sSGX %sHybrid\n", has_sgx ? "" : "No-", is_hybrid ? "" : "No-");

		if (has_sgx)
			decode_feature_control_msr();
	}

	if (max_level >= 0x15) {
/* …（同文件无关代码省略）… */
			}
			continue;
		}
		if (perf_pcore_set && CPU_ISSET_S(cpu, cpu_possible_setsize, perf_pcore_set)) {
			fd_l2_percpu[cpu] = open_perf_counter(cpu, perf_pmu_types.pcore, perf_model_support->first.refs, -1, PERF_FORMAT_GROUP);
			if (fd_l2_percpu[cpu] == -1) {
				warnx("%s(cpu%d, 0x%x, 0x%llx) REFS", __func__, cpu, perf_pmu_types.pcore, perf_model_support->first.refs);
/* …（同文件无关代码省略）… */
				free_fd_l2_percpu();
				return;
			}
		} else if (perf_ecore_set && CPU_ISSET_S(cpu, cpu_possible_setsize, perf_ecore_set)) {
			fd_l2_percpu[cpu] = open_perf_counter(cpu, perf_pmu_types.ecore, perf_model_support->second.refs, -1, PERF_FORMAT_GROUP);
			if (fd_l2_percpu[cpu] == -1) {
				warnx("%s(cpu%d, 0x%x, 0x%llx) REFS", __func__, cpu, perf_pmu_types.ecore, perf_model_support->second.refs);
/* …（同文件无关代码省略）… */
				free_fd_l2_percpu();
				return;
			}
		} else if (perf_lcore_set && CPU_ISSET_S(cpu, cpu_possible_setsize, perf_lcore_set)) {
			fd_l2_percpu[cpu] = open_perf_counter(cpu, perf_pmu_types.lcore, perf_model_support->third.refs, -1, PERF_FORMAT_GROUP);
			if (fd_l2_percpu[cpu] == -1) {
				warnx("%s(cpu%d, 0x%x, 0x%llx) REFS", __func__, cpu, perf_pmu_types.lcore, perf_model_support->third.refs);
/* …（同文件无关代码省略）… */
	return (ht_id - 1);
}

void topology_probe(bool startup)
{
	int i;
	int max_core_id = 0;
	int max_package_id = 0;
	int max_siblings = 0;

	/* Initialize num_cpus, max_cpu_num */
	set_max_cpu_num();
	topo.num_cpus = 0;
	for_all_proc_cpus(count_cpus);
	if (!summary_only)
/* …（同文件无关代码省略）… */
	cpu_present_set = CPU_ALLOC((topo.max_cpu_num + 1));
	if (cpu_present_set == NULL)
		err(3, "CPU_ALLOC");
	cpu_present_setsize = CPU_ALLOC_SIZE((topo.max_cpu_num + 1));
	CPU_ZERO_S(cpu_present_setsize, cpu_present_set);
	for_all_proc_cpus(mark_cpu_present);
	if (debug)
		print_cpu_set("present set", cpu_present_set);
/* …（同文件无关代码省略）… */
	cpu_possible_set = CPU_ALLOC((topo.max_cpu_num + 1));
	if (cpu_possible_set == NULL)
		err(3, "CPU_ALLOC");
	cpu_possible_setsize = CPU_ALLOC_SIZE((topo.max_cpu_num + 1));
	CPU_ZERO_S(cpu_possible_setsize, cpu_possible_set);
	initialize_cpu_set_from_sysfs(cpu_possible_set, "/sys/devices/system/cpu", "possible");
	if (debug)
		print_cpu_set("possible set", cpu_possible_set);
/* …（同文件无关代码省略）… */
	cpu_effective_set = CPU_ALLOC((topo.max_cpu_num + 1));
	if (cpu_effective_set == NULL)
		err(3, "CPU_ALLOC");
	cpu_effective_setsize = CPU_ALLOC_SIZE((topo.max_cpu_num + 1));
	CPU_ZERO_S(cpu_effective_setsize, cpu_effective_set);
	update_effective_set(startup);
	if (debug)
		print_cpu_set("effective set", cpu_effective_set);
/* …（同文件无关代码省略）… */
	cpu_allowed_set = CPU_ALLOC((topo.max_cpu_num + 1));
	if (cpu_allowed_set == NULL)
		err(3, "CPU_ALLOC");
	cpu_allowed_setsize = CPU_ALLOC_SIZE((topo.max_cpu_num + 1));
	CPU_ZERO_S(cpu_allowed_setsize, cpu_allowed_set);

	/*
	 * Validate and update cpu_allowed_set.
	 *
	 * Make sure all cpus in cpu_subset are also in cpu_present_set during startup.
	 * Give a warning when cpus in cpu_subset become unavailable at runtime.
	 * Give a warning when cpus are not effective because of cgroup setting.
	 *
	 * cpu_allowed_set is the intersection of cpu_present_set/cpu_effective_set/cpu_subset.
	 */
	for (i = 0; i < CPU_SUBSET_MAXCPUS; ++i) {
		if (cpu_subset && !CPU_ISSET_S(i, cpu_subset_size, cpu_subset))
			continue;

		if (!CPU_ISSET_S(i, cpu_present_setsize, cpu_present_set)) {
			if (cpu_subset) {
				/* cpus in cpu_subset must be in cpu_present_set during startup */
				if (startup)
					err(1, "cpu%d not present", i);
				else
					fprintf(stderr, "cpu%d not present\n", i);
			}
			continue;
		}

		if (CPU_COUNT_S(cpu_effective_setsize, cpu_effective_set)) {
			if (!CPU_ISSET_S(i, cpu_effective_setsize, cpu_effective_set)) {
				fprintf(stderr, "cpu%d not effective\n", i);
				continue;
			}
		}

		CPU_SET_S(i, cpu_allowed_setsize, cpu_allowed_set);
	}
	if (debug)
		print_cpu_set("allowed set", cpu_allowed_set);

	if (!CPU_COUNT_S(cpu_allowed_setsize, cpu_allowed_set))
		err(-ENODEV, "No valid cpus found");
	sched_setaffinity(0, cpu_allowed_setsize, cpu_allowed_set);

	/*
	 * Allocate and initialize cpu_affinity_set
	 */
	cpu_affinity_set = CPU_ALLOC((topo.max_cpu_num + 1));
	if (cpu_affinity_set == NULL)
		err(3, "CPU_ALLOC");
	cpu_affinity_setsize = CPU_ALLOC_SIZE((topo.max_cpu_num + 1));
	CPU_ZERO_S(cpu_affinity_setsize, cpu_affinity_set);

	for_all_proc_cpus(clear_ht_id);

/* …（同文件无关代码省略）… */
			continue;
		fprintf(outf,
			"cpu %d pkg %d die %d l3 %d node %d lnode %d module 0x%x core %d ht_id %d",
			i, cpus[i].package_id, cpus[i].die_id, cpus[i].l3_id,
			cpus[i].physical_node_id, cpus[i].logical_node_id, cpus[i].module_id, cpus[i].core_id, cpus[i].ht_id);
		fprintf(outf, " siblings");
		for (ht_id = 0; ht_id <= MAX_HT_ID; ++ht_id)
			fprintf(outf, " %d", cpus[i].ht_sibling_cpu_id[ht_id]);
/* …（同文件无关代码省略）… */
	if (counters->threads == NULL)
		goto error;

	for (i = 0; i < topo.max_cpu_num + 1; i++)
		(counters->threads)[i].cpu_id = -1;

	counters->cores = calloc(num_cores, sizeof(struct core_data));
/* …（同文件无关代码省略）… */
{
	int i;

	for (i = 0; i < topo.max_cpu_num + 1; ++i) {
		if (cpu_is_not_allowed(i))
			continue;
		master_cpu = i;
/* …（同文件无关代码省略）… */

		memset(domain_visited, 0, max_num_domains * sizeof(*domain_visited));

		for (int cpu = 0; cpu < topo.max_cpu_num + 1; ++cpu) {

			next_domain = cpu_to_domain(pinfo, cpu);

/* …（同文件无关代码省略）… */
int parse_telem_info_file(int fd_dir, const char *info_filename, const char *format, unsigned long *output)
{
	int fd_telem_info;
	FILE *file_telem_info;
	unsigned long value;

	fd_telem_info = openat(fd_dir, info_filename, O_RDONLY);
	if (fd_telem_info == -1)
		return -1;

	file_telem_info = fdopen(fd_telem_info, "r");
	if (file_telem_info == NULL) {
		close(fd_telem_info);
		return -1;
	}

	if (fscanf(file_telem_info, format, &value) != 1) {
		fclose(file_telem_info);
		return -1;
	}

	fclose(file_telem_info);

	*output = value;

	return 0;
}
/* …（同文件无关代码省略）… */
			break;
		}

		assert(offset == 0);

		fd_pmt = openat(fd_telem_dir, "telem", O_RDONLY);
		if (fd_pmt == -1)
			goto loop_cleanup_and_break;

		mmap_size = ROUND_UP_TO_PAGE_SIZE(size);
		mmio = mmap(0, mmap_size, PROT_READ, MAP_SHARED, fd_pmt, 0);
		if (mmio != MAP_FAILED) {
			if (debug)
/* …（同文件无关代码省略）… */
enum pmt_open_mode {
	PMT_OPEN_TRY,		/* Open failure is not an error. */
	PMT_OPEN_REQUIRED,	/* Open failure is a fatal error. */
};
/* …（同文件无关代码省略）… */
}

int pmt_add_counter(unsigned int guid, unsigned int seq, const char *name, enum pmt_datatype type,
		    unsigned int lsb, unsigned int msb, unsigned int offset, enum counter_scope scope,
		    enum counter_format format, unsigned int domain_id, enum pmt_open_mode mode)
{
	struct pmt_mmio *mmio;
	struct pmt_counter *pcounter;
/* …（同文件无关代码省略）… */
		return 1;
	}

	if (offset >= mmio->size) {
		if (mode != PMT_OPEN_TRY) {
			fprintf(stderr, "%s: offset %u outside of PMT MMIO size %u\n", __func__, offset, mmio->size);
			exit(1);
/* …（同文件无关代码省略）… */
			 * Set the scope to CPU, even though CWF report the counter per module.
			 * CPUs inside the same module will read from the same location, instead of reporting zeros.
			 *
			 * CWF with newer firmware might require a PMT_TYPE_XTAL_TIME intead of PMT_TYPE_TCORE_CLOCK.
			 */
			pmt_add_counter(PMT_CWF_MC1E_GUID, seq, "CPU%c1e", PMT_TYPE_TCORE_CLOCK,
					PMT_COUNTER_CWF_MC1E_LSB, PMT_COUNTER_CWF_MC1E_MSB, offset, SCOPE_CPU, FORMAT_DELTA, cpu_num, PMT_OPEN_TRY);
/* …（同文件无关代码省略）… */
void affinitize_child(void)
{
	/* Prefer cpu_possible_set, if available */
	if (sched_setaffinity(0, cpu_possible_setsize, cpu_possible_set)) {
		warn("sched_setaffinity cpu_possible_set");

		/* Otherwise, allow child to run on same cpu set as turbostat */
		if (sched_setaffinity(0, cpu_allowed_setsize, cpu_allowed_set))
			warn("sched_setaffinity cpu_allowed_set");
	}
}
/* …（同文件无关代码省略）… */
int get_and_dump_counters(void)
{
	int status;

	snapshot_proc_sysfs_files();
	status = for_all_cpus(get_counters, ODD_COUNTERS);
	if (status)
		return status;

	status = for_all_cpus(dump_counters, ODD_COUNTERS);
	if (status)
		return status;

	flush_output_stdout();

	return status;
}

void print_version()
{
	fprintf(outf, "turbostat version 2026.09.28 - Len Brown <lenb@kernel.org>\n");
}

#define COMMAND_LINE_SIZE 2048
/* …（同文件无关代码省略）… */
	fclose(fp);
}

struct msr_counter *find_msrp_by_name(struct msr_counter *head, char *name)
{
	struct msr_counter *mp;

/* …（同文件无关代码省略）… */
	return NULL;
}

int add_counter(unsigned int msr_num, char *path, char *name,
		unsigned int width, enum counter_scope scope, enum counter_type type, enum counter_format format, int flags, int id)
{
	struct msr_counter *msrp;

	if (no_msr && msr_num)
		errx(1, "Requested MSR counter 0x%x, but in --no-msr mode", msr_num);

	if (debug)
		fprintf(stderr, "%s(msr%d, %s, %s, width%d, scope%d, type%d, format%d, flags%x, id%d)\n",
			__func__, msr_num, path, name, width, scope, type, format, flags, id);

	switch (scope) {

/* …（同文件无关代码省略）… */
		struct sysfs_path *sp;

		sp = calloc(1, sizeof(struct sysfs_path));
		if (sp == NULL) {
			perror("calloc");
			exit(1);
		}
		strncpy(sp->path, path, PATH_BYTES - 1);
		sp->id = id;
		sp->next = msrp->sp;
/* …（同文件无关代码省略）… */
 * easily call re_initialize().
 */
struct perf_counter_info *make_perf_counter_info(const char *perf_device,
						 const char *perf_event,
						 const char *name,
						 unsigned int width, enum counter_scope scope, enum counter_type type, enum counter_format format)
{
	struct perf_counter_info *pinfo;

/* …（同文件无关代码省略）… */
int add_perf_counter(const char *perf_device, const char *perf_event, const char *name_buffer, unsigned int width,
		     enum counter_scope scope, enum counter_type type, enum counter_format format)
{
	struct perf_counter_info *pinfo;

	switch (scope) {
	case SCOPE_CPU:
		if (sys.added_thread_perf_counters >= MAX_ADDED_THREAD_COUNTERS) {
			warnx("ignoring thread counter perf/%s/%s", perf_device, perf_event);
			return -1;
		}
		break;

	case SCOPE_CORE:
		if (sys.added_core_perf_counters >= MAX_ADDED_CORE_COUNTERS) {
			warnx("ignoring core counter perf/%s/%s", perf_device, perf_event);
			return -1;
		}
		break;

	case SCOPE_PACKAGE:
		if (sys.added_package_perf_counters >= MAX_ADDED_PACKAGE_COUNTERS) {
			warnx("ignoring package counter perf/%s/%s", perf_device, perf_event);
			return -1;
		}
		break;
	}

	pinfo = make_perf_counter_info(perf_device, perf_event, name_buffer, width, scope, type, format);

	if (!pinfo)
		return -1;

	switch (scope) {
	case SCOPE_CPU:
		pinfo->next = sys.perf_tp;
		sys.perf_tp = pinfo;
		++sys.added_thread_perf_counters;
		break;

	case SCOPE_CORE:
		pinfo->next = sys.perf_cp;
		sys.perf_cp = pinfo;
		++sys.added_core_perf_counters;
		break;

	case SCOPE_PACKAGE:
		pinfo->next = sys.perf_pp;
		sys.perf_pp = pinfo;
		++sys.added_package_perf_counters;
		break;
	}

	// FIXME: we might not have debug here yet
	if (debug)
		fprintf(stderr, "%s: %s/%s, name: %s, scope%d\n", __func__, pinfo->device, pinfo->event, pinfo->name, pinfo->scope);

	return 0;
}
/* …（同文件无关代码省略）… */
void parse_add_command_msr(char *add_command)
{
	int msr_num = 0;
	char *path = NULL;
	char perf_device[PERF_DEV_NAME_BYTES] = "";
	char perf_event[PERF_EVT_NAME_BYTES] = "";
	char name_buffer[PERF_NAME_BYTES] = "";
	int width = 64;
	int fail = 0;
	enum counter_scope scope = SCOPE_CPU;
	enum counter_type type = COUNTER_CYCLES;
	enum counter_format format = FORMAT_DELTA;

	while (add_command) {

		if (sscanf(add_command, "msr0x%x", &msr_num) == 1)
			goto next;

		if (sscanf(add_command, "msr%d", &msr_num) == 1)
			goto next;

		BUILD_BUG_ON(ARRAY_SIZE(perf_device) <= 31);
		BUILD_BUG_ON(ARRAY_SIZE(perf_event) <= 31);
		if (sscanf(add_command, "perf/%31[^/]/%31[^,]", &perf_device[0], &perf_event[0]) == 2)
			goto next;

		if (*add_command == '/') {
			path = add_command;
			goto next;
		}

		if (sscanf(add_command, "u%d", &width) == 1) {
			if ((width == 32) || (width == 64))
				goto next;
			width = 64;
		}
		if (!strncmp(add_command, "cpu", strlen("cpu"))) {
			scope = SCOPE_CPU;
			goto next;
		}
		if (!strncmp(add_command, "core", strlen("core"))) {
			scope = SCOPE_CORE;
			goto next;
		}
		if (!strncmp(add_command, "package", strlen("package"))) {
			scope = SCOPE_PACKAGE;
			goto next;
		}
		if (!strncmp(add_command, "cycles", strlen("cycles"))) {
			type = COUNTER_CYCLES;
			goto next;
		}
		if (!strncmp(add_command, "seconds", strlen("seconds"))) {
			type = COUNTER_SECONDS;
			goto next;
		}
		if (!strncmp(add_command, "usec", strlen("usec"))) {
			type = COUNTER_USEC;
			goto next;
		}
		if (!strncmp(add_command, "raw", strlen("raw"))) {
			format = FORMAT_RAW;
			goto next;
		}
		if (!strncmp(add_command, "average", strlen("average"))) {
			format = FORMAT_AVERAGE;
			goto next;
		}
		if (!strncmp(add_command, "delta", strlen("delta"))) {
			format = FORMAT_DELTA;
			goto next;
		}
		if (!strncmp(add_command, "percent", strlen("percent"))) {
			format = FORMAT_PERCENT;
			goto next;
		}

		BUILD_BUG_ON(ARRAY_SIZE(name_buffer) <= 18);
		if (sscanf(add_command, "%18s,%*s", name_buffer) == 1) {
			char *eos;

			eos = strchr(name_buffer, ',');
			if (eos)
				*eos = '\0';
			goto next;
		}

next:
		add_command = strchr(add_command, ',');
		if (add_command) {
			*add_command = '\0';
			add_command++;
		}

	}
	if ((msr_num == 0) && (path == NULL) && (perf_device[0] == '\0' || perf_event[0] == '\0')) {
		fprintf(stderr, "--add: (msrDDD | msr0xXXX | /path_to_counter | perf/device/event) required\n");
		fail++;
	}

	/* Test for non-empty perf_device and perf_event */
	const bool is_perf_counter = perf_device[0] && perf_event[0];

	/* generate default column header */
	if (*name_buffer == '\0') {
		if (is_perf_counter) {
			snprintf(name_buffer, ARRAY_SIZE(name_buffer), "perf/%s", perf_event);
		} else {
			if (width == 32)
				sprintf(name_buffer, "M0x%x%s", msr_num, format == FORMAT_PERCENT ? "%" : "");
			else
				sprintf(name_buffer, "M0X%x%s", msr_num, format == FORMAT_PERCENT ? "%" : "");
		}
	}

	if (is_perf_counter) {
		if (add_perf_counter(perf_device, perf_event, name_buffer, width, scope, type, format))
			fail++;
	} else {
		if (add_counter(msr_num, path, name_buffer, width, scope, type, format, 0, 0))
			fail++;
	}

	if (fail) {
		help();
		exit(1);
	}
}
/* …（同文件无关代码省略）… */
bool starts_with(const char *str, const char *prefix)
{
	return strncmp(prefix, str, strlen(prefix)) == 0;
}
/* …（同文件无关代码省略）… */
int pmt_parse_from_path(const char *target_path, unsigned int *out_guid, unsigned int *out_seq)
{
	struct pmt_diriter_t pmt_iter;
	const struct dirent *dirname;
	struct stat stat, target_stat;
	int fd_telem_dir = -1;
	int fd_target_dir;
	unsigned int seq = 0;
	unsigned long guid, target_guid;
	int ret = -1;

	fd_target_dir = open(target_path, O_RDONLY | O_DIRECTORY);
	if (fd_target_dir == -1) {
		return -1;
	}

	if (fstat(fd_target_dir, &target_stat) == -1) {
		fprintf(stderr, "%s: Failed to stat the target: %s", __func__, strerror(errno));
		exit(1);
	}

	if (parse_telem_info_file(fd_target_dir, "guid", "%lx", &target_guid)) {
		fprintf(stderr, "%s: Failed to parse the target guid file: %s", __func__, strerror(errno));
		exit(1);
	}

	close(fd_target_dir);

	pmt_diriter_init(&pmt_iter);

	for (dirname = pmt_diriter_begin(&pmt_iter, SYSFS_TELEM_PATH); dirname != NULL; dirname = pmt_diriter_next(&pmt_iter)) {

		fd_telem_dir = openat(dirfd(pmt_iter.dir), dirname->d_name, O_RDONLY | O_DIRECTORY);
		if (fd_telem_dir == -1)
			continue;

		if (parse_telem_info_file(fd_telem_dir, "guid", "%lx", &guid)) {
			fprintf(stderr, "%s: Failed to parse the guid file: %s", __func__, strerror(errno));
			continue;
		}

		if (fstat(fd_telem_dir, &stat) == -1) {
			fprintf(stderr, "%s: Failed to stat %s directory: %s", __func__, dirname->d_name, strerror(errno));
			continue;
		}

		/*
		 * If reached the same directory as target, exit the loop.
		 * Seq has the correct value now.
		 */
		if (stat.st_dev == target_stat.st_dev && stat.st_ino == target_stat.st_ino) {
			ret = 0;
			break;
		}

		/*
		 * If reached directory with the same guid,
		 * but it's not the target directory yet,
		 * increment seq and continue the search.
		 */
		if (guid == target_guid)
			++seq;

		close(fd_telem_dir);
		fd_telem_dir = -1;
	}

	pmt_diriter_remove(&pmt_iter);

	if (fd_telem_dir != -1)
		close(fd_telem_dir);

	if (!ret) {
		*out_guid = target_guid;
		*out_seq = seq;
	}

	return ret;
}
/* …（同文件无关代码省略）… */
void parse_add_command_pmt(char *add_command)
{
	char *name = NULL;
	char *type_name = NULL;
	char *format_name = NULL;
	char *direct_path = NULL;
	static const char direct_path_prefix[] = "path=";
	unsigned int offset;
	unsigned int lsb;
	unsigned int msb;
	unsigned int guid;
	unsigned int seq = 0;	/* By default, pick first file in a sequence with a given GUID. */
	unsigned int domain_id;
	enum counter_scope scope = 0;
	enum pmt_datatype type = PMT_TYPE_RAW;
	enum counter_format format = FORMAT_RAW;
	bool has_offset = false;
	bool has_lsb = false;
	bool has_msb = false;
	bool has_format = true;	/* Format has a default value. */
	bool has_guid = false;
	bool has_scope = false;
	bool has_type = true;	/* Type has a default value. */

	/* Consume the "pmt," prefix. */
	add_command = strchr(add_command, ',');
	if (!add_command) {
		help();
		exit(1);
	}
	++add_command;

	while (add_command) {
		if (starts_with(add_command, "name=")) {
			name = add_command + strlen("name=");
			goto next;
		}

		if (starts_with(add_command, "type=")) {
			type_name = add_command + strlen("type=");
			goto next;
		}

		if (starts_with(add_command, "domain=")) {
			const size_t prefix_len = strlen("domain=");

			if (sscanf(add_command + prefix_len, "cpu%u", &domain_id) == 1) {
				scope = SCOPE_CPU;
				has_scope = true;
			} else if (sscanf(add_command + prefix_len, "core%u", &domain_id) == 1) {
				scope = SCOPE_CORE;
				has_scope = true;
			} else if (sscanf(add_command + prefix_len, "package%u", &domain_id) == 1) {
				scope = SCOPE_PACKAGE;
				has_scope = true;
			}

			if (!has_scope) {
				printf("%s: invalid value for scope. Expected cpu%%u, core%%u or package%%u.\n", __func__);
				exit(1);
			}

			goto next;
		}

		if (starts_with(add_command, "format=")) {
			format_name = add_command + strlen("format=");
			goto next;
		}

		if (sscanf(add_command, "offset=%u", &offset) == 1) {
			has_offset = true;
			goto next;
		}

		if (sscanf(add_command, "lsb=%u", &lsb) == 1) {
			has_lsb = true;
			goto next;
		}

		if (sscanf(add_command, "msb=%u", &msb) == 1) {
			has_msb = true;
			goto next;
		}

		if (sscanf(add_command, "guid=%x", &guid) == 1) {
			has_guid = true;
			goto next;
		}

		if (sscanf(add_command, "seq=%x", &seq) == 1)
			goto next;

		if (strncmp(add_command, direct_path_prefix, strlen(direct_path_prefix)) == 0) {
			direct_path = add_command + strlen(direct_path_prefix);
			goto next;
		}
next:
		add_command = strchr(add_command, ',');
		if (add_command) {
			*add_command = '\0';
			add_command++;
		}
	}

	if (!name) {
		printf("%s: missing %s\n", __func__, "name");
		exit(1);
	}

	if (strlen(name) >= PMT_COUNTER_NAME_SIZE_BYTES) {
		printf("%s: name has to be at most %d characters long\n", __func__, PMT_COUNTER_NAME_SIZE_BYTES);
		exit(1);
	}

	if (format_name) {
		has_format = false;

		if (strcmp("raw", format_name) == 0) {
			format = FORMAT_RAW;
			has_format = true;
		}

		if (strcmp("average", format_name) == 0) {
			format = FORMAT_AVERAGE;
			has_format = true;
		}

		if (strcmp("delta", format_name) == 0) {
			format = FORMAT_DELTA;
			has_format = true;
		}

		if (!has_format) {
			fprintf(stderr, "%s: Invalid format %s. Expected raw, average or delta\n", __func__, format_name);
			exit(1);
		}
	}

	if (type_name) {
		has_type = false;

		if (strcmp("raw", type_name) == 0) {
			type = PMT_TYPE_RAW;
			has_type = true;
		}

		if (strcmp("txtal_time", type_name) == 0) {
			type = PMT_TYPE_XTAL_TIME;
			has_type = true;
		}

		if (strcmp("tcore_clock", type_name) == 0) {
			type = PMT_TYPE_TCORE_CLOCK;
			has_type = true;
		}

		if (!has_type) {
			printf("%s: invalid %s: %s\n", __func__, "type", type_name);
			exit(1);
		}
	}

	if (!has_offset) {
		printf("%s : missing %s\n", __func__, "offset");
		exit(1);
	}

	if (!has_lsb) {
		printf("%s: missing %s\n", __func__, "lsb");
		exit(1);
	}

	if (!has_msb) {
		printf("%s: missing %s\n", __func__, "msb");
		exit(1);
	}

	if (direct_path && has_guid) {
		printf("%s: path and guid+seq parameters are mutually exclusive\nnotice: passed guid=0x%x and path=%s\n", __func__, guid, direct_path);
		exit(1);
	}

	if (direct_path) {
		if (pmt_parse_from_path(direct_path, &guid, &seq)) {
			printf("%s: failed to parse PMT file from %s\n", __func__, direct_path);
			exit(1);
		}

		/* GUID was just infered from the direct path. */
		has_guid = true;
	}

	if (!has_guid) {
		printf("%s: missing %s\n", __func__, "guid or path");
		exit(1);
	}

	if (!has_scope) {
		printf("%s: missing %s\n", __func__, "scope");
		exit(1);
	}

	if (lsb > msb) {
		printf("%s: lsb > msb doesn't make sense\n", __func__);
		exit(1);
	}

	pmt_add_counter(guid, seq, name, type, lsb, msb, offset, scope, format, domain_id, PMT_OPEN_REQUIRED);
}
/* …（同文件无关代码省略）… */
void parse_add_command(char *add_command)
{
	if (strncmp(add_command, "pmt", strlen("pmt")) == 0)
		return parse_add_command_pmt(add_command);
	return parse_add_command_msr(add_command);
}
/* …（同文件无关代码省略）… */

/*
 * parse cpuset with following syntax
 * 1,2,4..6,8-10 and set bits in cpu_subset
 */
void parse_cpu_command(char *optarg)
{
	if (!strcmp(optarg, "core")) {
		if (cpu_subset)
			goto error;
		show_core_only++;
		return;
	}
	if (!strcmp(optarg, "package")) {
		if (cpu_subset)
			goto error;
		show_pkg_only++;
		return;
	}
	if (show_core_only || show_pkg_only)
		goto error;

	cpu_subset = CPU_ALLOC(CPU_SUBSET_MAXCPUS);
	if (cpu_subset == NULL)
		err(3, "CPU_ALLOC");
	cpu_subset_size = CPU_ALLOC_SIZE(CPU_SUBSET_MAXCPUS);

	CPU_ZERO_S(cpu_subset_size, cpu_subset);

	if (parse_cpu_str(optarg, cpu_subset, cpu_subset_size))
		goto error;

	return;

/* …（同文件无关代码省略）… */
void cmdline(int argc, char **argv)
{
	int opt;
	int option_index = 0;
	static struct option long_options[] = {
		{ "add", required_argument, 0, 'a' },
		{ "cpu", required_argument, 0, 'c' },
		{ "Dump", no_argument, 0, 'D' },
		{ "debug", no_argument, 0, 'd' },	/* internal, not documented */
		{ "enable", required_argument, 0, 'e' },
		{ "force", no_argument, 0, 'f' },
		{ "interval", required_argument, 0, 'i' },
		{ "IPC", no_argument, 0, 'I' },
		{ "num_iterations", required_argument, 0, 'n' },
		{ "header_iterations", required_argument, 0, 'N' },
		{ "help", no_argument, 0, 'h' },
		{ "hide", required_argument, 0, 'H' },	// meh, -h taken by --help
		{ "Joules", no_argument, 0, 'J' },
		{ "list", no_argument, 0, 'l' },
		{ "out", required_argument, 0, 'o' },
		{ "quiet", no_argument, 0, 'q' },
		{ "no-msr", no_argument, 0, 'M' },
		{ "no-perf", no_argument, 0, 'P' },
		{ "show", required_argument, 0, 's' },
		{ "Summary", no_argument, 0, 'S' },
		{ "TCC", required_argument, 0, 'T' },
		{ "version", no_argument, 0, 'v' },
		{ 0, 0, 0, 0 }
	};

	progname = argv[0];

	/*
	 * Parse some options early, because they may make other options invalid,
	 * like adding the MSR counter with --add and at the same time using --no-msr.
	 */
	while ((opt = getopt_long_only(argc, argv, "+:MP", long_options, &option_index)) != -1) {
		switch (opt) {
		case 'M':
			no_msr = 1;
			break;
		case 'P':
			no_perf = 1;
			break;
		default:
			break;
		}
	}
	optind = 0;

	while ((opt = getopt_long_only(argc, argv, "+C:c:Dde:hi:Jn:N:o:qMPST:v", long_options, &option_index)) != -1) {
		switch (opt) {
		case 'a':
			parse_add_command(optarg);
			break;
		case 'c':
			parse_cpu_command(optarg);
			break;
		case 'D':
			dump_only++;
			/*
			 * Force the no_perf early to prevent using it as a source.
			 * User asks for raw values, but perf returns them relative
			 * to the opening of the file descriptor.
			 */
			no_perf = 1;
			break;
		case 'e':
			/* --enable specified counter, without clearning existing list */
			bic_lookup(&bic_enabled, optarg, SHOW_LIST);
			break;
		case 'f':
			force_load++;
			break;
		case 'd':
			debug++;
			bic_set_all(&bic_enabled);
			break;
		case 'H':
			/*
			 * --hide: do not show those specified
			 *  multiple invocations simply clear more bits in enabled mask
			 */
			{
				cpu_set_t bic_group_hide;

				BIC_INIT(&bic_group_hide);

				bic_lookup(&bic_group_hide, optarg, HIDE_LIST);
				bic_clear_bits(&bic_enabled, &bic_group_hide);
			}
			break;
		case 'h':
			help();
			exit(1);
		case 'i':
			{
				double interval = strtod(optarg, NULL);

				if (interval < 0.001) {
					fprintf(outf, "interval %f seconds is too small\n", interval);
					exit(2);
				}

				interval_tv.tv_sec = interval_ts.tv_sec = interval;
				interval_tv.tv_usec = (interval - interval_tv.tv_sec) * 1000000;
				interval_ts.tv_nsec = (interval - interval_ts.tv_sec) * 1000000000;
			}
			break;
		case 'J':
			rapl_joules++;
			break;
		case 'l':
			bic_set_all(&bic_enabled);
			list_header_only++;
			quiet++;
			break;
		case 'o':
			outf = fopen_or_die(optarg, "w");
			break;
		case 'q':
			quiet = 1;
			break;
		case 'M':
		case 'P':
			/* Parsed earlier */
			break;
		case 'n':
			num_iterations = strtoul(optarg, NULL, 0);
			errno = 0;

			if (errno || num_iterations == 0)
				errx(-1, "invalid iteration count: %s", optarg);
			break;
		case 'N':
			header_iterations = strtoul(optarg, NULL, 0);
			errno = 0;

			if (errno || header_iterations == 0)
				errx(-1, "invalid header iteration count: %s", optarg);
			break;
		case 's':
			/*
			 * --show: show only those specified
			 *  The 1st invocation will clear and replace the enabled mask
			 *  subsequent invocations can add to it.
			 */
			if (shown == 0)
				BIC_INIT(&bic_enabled);
			bic_lookup(&bic_enabled, optarg, SHOW_LIST);
			shown = 1;
			break;
		case 'S':
			summary_only++;
			break;
		case 'T':
			tj_max_override = atoi(optarg);
			break;
		case 'v':
			print_version();
			exit(0);
			break;
		default:
			help();
			exit(1);
		}
	}
}
/* …（同文件无关代码省略）… */

	ret = write(fd, "0\n", 2);
	if (ret == -1)
		perror("Can't update cgroup\n");

	close(fd);

skip_cgroup_setting:
	outf = stderr;
	cmdline(argc, argv);

/* …（同文件无关代码省略）… */
		msr_sum_record();

	/* dump counters and exit */
	if (dump_only)
		return get_and_dump_counters();

	/* list header and exit */
	if (list_header_only) {
