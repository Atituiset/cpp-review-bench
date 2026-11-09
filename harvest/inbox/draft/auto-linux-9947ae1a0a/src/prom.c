// AUTO-DRAFT from torvalds/linux PR #2651f6deb3326129a052897df193052dc28ea94f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
char **prom_argv;
char **prom_envp;

void __init prom_init_cmdline(void)
{
	int i;
/* …（同文件无关代码省略）… */
	if (!memsize_str || kstrtoul(memsize_str, 0, &memsize))
		memsize = SZ_64M; /* minimum memsize is 64MB RAM */

	memblock_add(0, memsize);
}

static inline unsigned char str2hexnum(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0; /* foo */
}
/* …（同文件无关代码省略）… */
static inline void str2eaddr(unsigned char *ea, unsigned char *str)
{
	int i;

	for (i = 0; i < 6; i++) {
		unsigned char num;

		if ((*str == '.') || (*str == ':'))
			str++;
		num  = str2hexnum(*str++) << 4;
		num |= str2hexnum(*str++);
		ea[i] = num;
	}
}

int __init prom_get_ethernet_addr(char *ethernet_addr)
{
	char *ethaddr_str;

/* …（同文件无关代码省略）… */
		/* Check command line */
		ethaddr_str = strstr(arcs_cmdline, "ethaddr=");
		if (!ethaddr_str)
			return -1;

		ethaddr_str += strlen("ethaddr=");
	}

	str2eaddr(ethernet_addr, ethaddr_str);

	return 0;
}
