// AUTO-DRAFT from torvalds/linux PR #06ac073129191a01d77933ed381f89c67674db6f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static unsigned long get_align_mask(struct file *filp)
{
	if (filp && is_file_hugepages(filp))
		return huge_page_mask_align(filp);
	/* handle 32- and 64-bit case with a single conditional */
	if (va_align.flags < 0 || !(va_align.flags & (2 - mmap_is_ia32())))
		return 0;

	if (!(current->flags & PF_RANDOMIZE))
		return 0;

	return va_align.mask;
}
/* …（同文件无关代码省略）… */
 * value before calling vm_unmapped_area() or ORed directly to the
 * address.
 */
static unsigned long get_align_bits(void)
{
	return va_align.bits & get_align_mask(NULL);
}

static int __init control_va_addr_alignment(char *str)
{
	/* guard against enabling this on other CPU families */
	if (va_align.flags < 0)
		return 1;

	if (*str == 0)
		return 1;

	if (!strcmp(str, "32"))
		va_align.flags = ALIGN_VA_32;
	else if (!strcmp(str, "64"))
		va_align.flags = ALIGN_VA_64;
	else if (!strcmp(str, "off"))
		va_align.flags = 0;
	else if (!strcmp(str, "on"))
		va_align.flags = ALIGN_VA_32 | ALIGN_VA_64;
	else
		pr_warn("invalid option value: 'align_va_addr=%s'\n", str);

	return 1;
}
/* …（同文件无关代码省略）… */
	}
	if (filp) {
		info.align_mask = get_align_mask(filp);
		info.align_offset += get_align_bits();
	}

	return vm_unmapped_area(&info);
/* …（同文件无关代码省略）… */

	if (filp) {
		info.align_mask = get_align_mask(filp);
		info.align_offset += get_align_bits();
	}
	addr = vm_unmapped_area(&info);
	if (!(addr & ~PAGE_MASK))
