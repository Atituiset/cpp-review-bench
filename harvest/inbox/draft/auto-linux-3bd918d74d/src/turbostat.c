// AUTO-DRAFT from torvalds/linux PR #cbc08c20479c6da3db077f9b170bb8cc2d09df20
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define	NAME_BYTES 20
/* …（同文件无关代码省略）… */
enum counter_scope { SCOPE_CPU, SCOPE_CORE, SCOPE_PACKAGE };
enum counter_type { COUNTER_ITEMS, COUNTER_CYCLES, COUNTER_SECONDS, COUNTER_USEC, COUNTER_K2M };
enum counter_format { FORMAT_RAW, FORMAT_DELTA, FORMAT_PERCENT, FORMAT_AVERAGE };
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
/* …（同文件无关代码省略）… */
#define SET_BIC(COUNTER_NUMBER, set) CPU_SET(COUNTER_NUMBER, set)
/* …（同文件无关代码省略）… */
#define BIC_PRESENT(COUNTER_NUMBER) SET_BIC(COUNTER_NUMBER, &bic_present)
#define BIC_NOT_PRESENT(COUNTER_NUMBER) CPU_CLR(COUNTER_NUMBER, &bic_present)
/* …（同文件无关代码省略）… */
#define MAX(a, b) ((a) > (b) ? (a) : (b))
/* …（同文件无关代码省略）… */
#define CPU_SUBSET_MAXCPUS	8192	/* need to use before probe... */
/* …（同文件无关代码省略）… */
#define MAX_ADDED_THREAD_COUNTERS 24
#define MAX_ADDED_CORE_COUNTERS 8
#define MAX_ADDED_PACKAGE_COUNTERS 16
/* …（同文件无关代码省略）… */
#define PMT_COUNTER_NAME_SIZE_BYTES      16
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
int pmt_telemdir_filter(const struct dirent *e)
{
	unsigned int dummy;

	return sscanf(e->d_name, "telem%u", &dummy);
}
/* …（同文件无关代码省略）… */
int pmt_telemdir_sort(const struct dirent **a, const struct dirent **b)
{
	unsigned int aidx = 0, bidx = 0;

	sscanf((*a)->d_name, "telem%u", &aidx);
	sscanf((*b)->d_name, "telem%u", &bidx);

	return (aidx > bidx) ? 1 : (aidx < bidx) ? -1 : 0;
}
/* …（同文件无关代码省略）… */
			return NULL;

		num_names = scandir(pmt_root_path, &iter->namelist, pmt_telemdir_filter, pmt_telemdir_sort);
		if (num_names == -1)
			return NULL;
	}

	iter->current_name_idx = 0;
/* …（同文件无关代码省略）… */
	iter->num_names = 0;
	iter->current_name_idx = 0;

	closedir(iter->dir);
	iter->dir = NULL;
}

unsigned int pmt_counter_get_width(const struct pmt_counter *p)
/* …（同文件无关代码省略）… */
int cpu_is_not_allowed(int cpu)
{
	if (cpu < 0)
		return 1;

	return !CPU_ISSET_S(cpu, cpu_allowed_setsize, cpu_allowed_set);
}
/* …（同文件无关代码省略）… */
#define GLOBAL_CORE_ID(core_id, pkg_id)	(core_id + pkg_id * (topo.max_core_id + 1))
/* …（同文件无关代码省略）… */
		if (p->gfx_rc6_ms == -1) {	/* detect GFX counter reset */
			outp += sprintf(outp, "%s**.**", (printed++ ? delim : ""));
		} else {
			outp += sprintf(outp, "%s%.2f", (printed++ ? delim : ""), p->gfx_rc6_ms / 10.0 / interval_float);
		}
	}

/* …（同文件无关代码省略）… */
		if (p->sam_mc6_ms == -1) {	/* detect GFX counter reset */
			outp += sprintf(outp, "%s**.**", (printed++ ? delim : ""));
		} else {
			outp += sprintf(outp, "%s%.2f", (printed++ ? delim : ""), p->sam_mc6_ms / 10.0 / interval_float);
		}
	}

/* …（同文件无关代码省略）… */
static inline int get_rapl_num_domains(void)
{
	if (!platform->has_per_core_rapl)
		return topo.num_packages;

	return GLOBAL_CORE_ID(topo.max_core_id, topo.num_packages) + 1;
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
	if (!msr_counter_info)
		return;

	for (int cpu = 0; cpu < topo.max_cpu_num; ++cpu) {
		if (msr_counter_info[cpu].fd_perf != -1)
			close(msr_counter_info[cpu].fd_perf);
	}
/* …（同文件无关代码省略）… */
static int parse_cpu_str(char *cpu_str, cpu_set_t *cpu_set, int cpu_set_size)
{
	unsigned int start, end;
	char *next = cpu_str;

	while (next && *next) {

		if (*next == '-')	/* no negative cpu numbers */
			return 1;

		if (*next == '\0' || *next == '\n')
			break;

		start = strtoul(next, &next, 10);

		if (start >= CPU_SUBSET_MAXCPUS)
			return 1;
		CPU_SET_S(start, cpu_set_size, cpu_set);

		if (*next == '\0' || *next == '\n')
			break;

		if (*next == ',') {
			next += 1;
			continue;
		}

		if (*next == '-') {
			next += 1;	/* start range */
		} else if (*next == '.') {
			next += 1;
			if (*next == '.')
				next += 1;	/* start range */
			else
				return 1;
		}

		end = strtoul(next, &next, 10);
		if (end <= start)
			return 1;

		while (++start <= end) {
			if (start >= CPU_SUBSET_MAXCPUS)
				return 1;
			CPU_SET_S(start, cpu_set_size, cpu_set);
		}

		if (*next == ',')
			next += 1;
		else if (*next != '\0' && *next != '\n')
			return 1;
	}

	return 0;
}
/* …（同文件无关代码省略）… */

#define PATH_EFFECTIVE_CPUS	"/sys/fs/cgroup/cpuset.cpus.effective"

static char cpu_effective_str[1024];

static int update_effective_str(bool startup)
{
	FILE *fp;
	char *pos;
	char buf[1024];
	int ret;

	if (cpu_effective_str[0] == '\0' && !startup)
		return 0;

	fp = fopen(PATH_EFFECTIVE_CPUS, "r");
	if (!fp)
		return 0;

	pos = fgets(buf, 1024, fp);
	if (!pos)
		err(1, "%s: file read failed", PATH_EFFECTIVE_CPUS);

	fclose(fp);

	ret = strncmp(cpu_effective_str, buf, 1024);
	if (!ret)
		return 0;

	strncpy(cpu_effective_str, buf, 1024);
	return 1;
}

/* …（同文件无关代码省略）… */
	return ret;
}

char cpuset_buf[1024];
int initialize_cpu_set_from_sysfs(cpu_set_t *cpu_
