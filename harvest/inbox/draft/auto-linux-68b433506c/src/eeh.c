// AUTO-DRAFT from torvalds/linux PR #7ccb044c27720d48175577080b851f047ddcfd1c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdint.h>
  // <<< BUG ANCHOR
	struct pci_controller *hose;
	uint32_t phbid, pe_no;
	struct eeh_pe *pe;
	char buf[20];
	int ret;

	ret = simple_write_to_buffer(buf, sizeof(buf), ppos, user_buf, count);
	if (!ret)
		return -EFAULT;
