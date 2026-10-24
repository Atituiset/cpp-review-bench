// AUTO-DRAFT from torvalds/linux PR #845f394e462fb1bd67b9779c467be478631a4daf
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
  // <<< BUG ANCHOR
#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/printk.h>
/* …（同文件无关代码省略）… */

	int boot_int;

	unsigned int uint_0001;

	char string_0001[65];
/* …（同文件无关代码省略）… */

	.boot_int = 0,

	.uint_0001 = 314,

	.string_0001 = "(none)",
/* …（同文件无关代码省略）… */
		.extra1		= SYSCTL_ZERO,
		.extra2         = SYSCTL_ONE,
	},
	{
		.procname	= "uint_0001",
		.data		= &test_data.uint_0001,
/* …（同文件无关代码省略）… */

static void __exit test_sysctl_exit(void)
{
	kfree(test_data.bitmap_0001);
	for (int i = 0; i < TEST_H_SIZE; i++) {
		if (ctl_headers[i])
			unregister_sysctl_table(ctl_headers[i]);
	}
}

module_exit(test_sysctl_exit);
