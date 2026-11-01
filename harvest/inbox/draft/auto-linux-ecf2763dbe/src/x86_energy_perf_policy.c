// AUTO-DRAFT from torvalds/linux PR #cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
 */

#define _GNU_SOURCE
#include MSRHEADER
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <getopt.h>
#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <limits.h>
#include <stdlib.h>
/* …（同文件无关代码省略）… */
#define	OPTARG_NORMAL			(INT_MAX - 1)
#define	OPTARG_POWER			(INT_MAX - 2)
#define	OPTARG_BALANCE_POWER		(INT_MAX - 3)
#define	OPTARG_BALANCE_PERFORMANCE	(INT_MAX - 4)
#define	OPTARG_PERFORMANCE		(INT_MAX - 5)

struct msr_hwp_cap {
	unsigned char highest;
	unsigned char guaranteed;
/* …（同文件无关代码省略）… */
struct msr_hwp_request {
	unsigned char hwp_min;
	unsigned char hwp_max;
	unsigned char hwp_desired;
	unsigned char hwp_epp;
	unsigned int hwp_window;
	unsigned char hwp_use_pkg;
} req_update;
/* …（同文件无关代码省略）… */
int soc_slider_balance;
int soc_slider_offset;
char platform_profile[64];

#define PATH_TO_CPU "/sys/devices/system/cpu/"
#define SYSFS_PATH_MAX 255
/* …（同文件无关代码省略）… */
#define PATH_PLATFORM_PROFILE "/sys/class/platform-profile/platform-profile-0/profile"
/* …（同文件无关代码省略）… */
void usage(void)
{
	fprintf(stderr, "%s [options] [scope][field value]\n", progname);
	fprintf(stderr, "scope: --cpu cpu-list [--hwp-use-pkg #] | --pkg pkg-list\n");
	fprintf(stderr, "field: --all | --epb | --hwp-epp | --hwp-min | --hwp-max | --hwp-desired\n");
	fprintf(stderr, "other: --hwp-enable | --turbo-enable (0 | 1) | --help | --force\n");
	fprintf(stderr, "soc-slider: --soc-slider-balance # | --soc-slider-offset # | --platform-profile <name>\n");
	fprintf(stderr, "value: ( # | \"normal\" | \"performance\" | \"balance-performance\" | \"balance-power\"| \"power\")\n");
	fprintf(stderr, "--hwp-window usec\n");

	fprintf(stderr, "Specify only Energy Performance BIAS (legacy usage):\n");
	fprintf(stderr, "%s: [-c cpu] [-v] (-r | policy-value )\n", progname);

	exit(1);
}
/* …（同文件无关代码省略）… */
int ratio_2_msr_perf(int ratio)
{
	int msr_perf;

	if (!bdx_highest_ratio)
		return ratio;

	msr_perf = ratio * 255 / bdx_highest_ratio;

	if (debug)
		fprintf(stderr, "%d = ratio_to_msr_perf(%d)\n", msr_perf, ratio);
/* …（同文件无关代码省略）… */
int msr_perf_2_ratio(int msr_perf)
{
	int ratio;
	double d;

	if (!bdx_highest_ratio)
		return msr_perf;

	d = (double)msr_perf * (double)bdx_highest_ratio / 255.0;
	d = d + 0.5;		/* round */
	ratio = (int)d;

	if (debug)
		fprintf(stderr, "%d = msr_perf_ratio(%d) {%f}\n", ratio, msr_perf, d);

	return ratio;
}
/* …（同文件无关代码省略）… */
		return ENERGY_PERF_BIAS_PERFORMANCE;
	}
	if (i < 0 || i > ENERGY_PERF_BIAS_POWERSAVE)
		errx(1, "--epb must be from 0 to 15");
	return i;
}

/* …（同文件无关代码省略）… */
	return i;
}

int parse_cmdline_hwp_window(int i)
{
	unsigned int exponent;

/* …（同文件无关代码省略）… */
	case OPTARG_PERFORMANCE:
		return 0;
	}
	if (i < 0 || i > 1270000000) {
		fprintf(stderr, "--hwp-window: 0 for auto; 1 - 1270000000 usec for window duration\n");
		usage();
	}
	for (exponent = 0;; ++exponent) {
		if (debug)
			printf("%d 10^%d\n", i, exponent);

		if (i <= 127)
			break;

		i = i / 10;
	}
	if (debug)
		fprintf(stderr, "%d*10^%d: 0x%x\n", i, exponent, (exponent << 7) | i);

	return (exponent << 7) | i;
}
/* …（同文件无关代码省略）… */
	case OPTARG_PERFORMANCE:
		return HWP_EPP_PERFORMANCE;
	}
	if (i < 0 || i > 0xff) {
		fprintf(stderr, "--hwp-epp must be from 0 to 0xff\n");
		usage();
	}
	return i;
/* …（同文件无关代码省略）… */
int parse_cmdline_turbo(int i)
{
	update_turbo = 1;

	switch (i) {
	case OPTARG_POWER:
		return 0;
	case OPTARG_NORMAL:
	case OPTARG_BALANCE_POWER:
	case OPTARG_BALANCE_PERFORMANCE:
	case OPTARG_PERFORMANCE:
		return 1;
	}
	if (i < 0 || i > 1) {
		fprintf(stderr, "--turbo-enable: 1 to enable, 0 to disable\n");
		usage();
	}
	return i;
}

int parse_optarg_string(char *s)
{
	int i;
	char *endptr;

	if (!strncmp(s, "default", 7))
		return OPTARG_NORMAL;

	if (!strncmp(s, "normal", 6))
		return OPTARG_NORMAL;

	if (!strncmp(s, "power", 9))
		return OPTARG_POWER;

	if (!strncmp(s, "balance-power", 17))
		return OPTARG_BALANCE_POWER;

	if (!strncmp(s, "balance-performance", 19))
		return OPTARG_BALANCE_PERFORMANCE;

	if (!strncmp(s, "performance", 11))
		return OPTARG_PERFORMANCE;

	i = strtol(s, &endptr, 0);
	if (s == endptr) {
		fprintf(stderr, "no digits in \"%s\"\n", s);
		usage();
	}
	if (i == LONG_MIN || i == LONG_MAX)
		errx(-1, "%s", s);

	if (i > 0xFF)
		errx(-1, "%d (0x%x) must be < 256", i, i);

	if (i < 0)
		errx(-1, "%d (0x%x) must be >= 0", i, i);
	return i;
}

/* …（同文件无关代码省略）… */
			int end_cpu;

			startp++;
			end_cpu = strtol(startp, &endp, 10);
			if (startp == endp)
				continue;

			while (cpu <= end_cpu) {
				if (cpu > max_cpu_num)
/* …（同文件无关代码省略）… */
				break;
		}

		cpu = strtol(startp, &endp, 10);
		if (startp == endp)
			errx(1, "--cpu cpu-set: confused by '%s'", startp);
		if (cpu > max_cpu_num)
			errx(1, "Requested cpu%d exceeds max cpu%d", cpu, max_cpu_num);
		CPU_SET_S(cpu, cpu_setsize, cpu_selected_set);
		startp = endp;
	}
/* …（同文件无关代码省略）… */
			int end_pkg;

			startp++;
			end_pkg = strtol(startp, &endp, 10);
			if (startp == endp)
				continue;

			while (pkg <= end_pkg) {
				if (pkg > max_pkg_num)
					errx(1, "Requested pkg%d exceeds max pkg%d", pkg, max_pkg_num);
				pkg_selected_set |= 1 << pkg;
				pkg++;
			}
			startp = endp;
/* …（同文件无关代码省略）… */
			return;
		}

		pkg = strtol(startp, &endp, 10);
		if (pkg > max_pkg_num)
			errx(1, "Requested pkg%d Exceeds max pkg%d", pkg, max_pkg_num);
		pkg_selected_set |= 1 << pkg;
		startp = endp;
	}
}
/* …（同文件无关代码省略）… */
{
	int pkg_num;

	for (pkg_num = 0; pkg_num <= max_pkg_num; ++pkg_num) {
		if (pkg_set & (1UL << pkg_num))
			func(pkg_num);
	}
}
/* …（同文件无关代码省略）… */
	char *endp;
	long val;

	val = strtol(s, &endp, 0);
	if (endp == s || errno == ERANGE)
		return -1;
/* …（同文件无关代码省略）… */

void print_version(void)
{
	printf("x86_energy_perf_p
