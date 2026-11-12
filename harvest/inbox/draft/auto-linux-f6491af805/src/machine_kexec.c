// AUTO-DRAFT from torvalds/linux PR #79eb6679ef6a0d5ea68da53a8321fb1dc72299b9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>

#include <linux/mm.h>
#include <linux/of_fdt.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/sched/task_stack.h>

/* …（同文件无关代码省略）… */
#define KEXEC_CMDLINE_ADDR	TO_CACHE(0x108000UL)
/* …（同文件无关代码省略）… */
		 */
		memcpy((void *)KEXEC_CMDLINE_ADDR, (void *)kimage->arch.cmdline_ptr,
					strlen((char *)kimage->arch.cmdline_ptr) + 1);
		kimage->arch.cmdline_ptr = (unsigned long)KEXEC_CMDLINE_ADDR;
	} else {
		/* Find the command line */
