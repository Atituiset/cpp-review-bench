// AUTO-DRAFT from torvalds/linux PR #864c07de0c7dcbca58b61625dfa2e461ae63b242
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
if (count >= PAGE_SIZE)
		return -EINVAL;

	buf = (char *)__get_free_page(GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	if (copy_from_user(buf, user_buf, count)) {
		free_page((unsigned long) buf);
		return -EFAULT;
	}
	/* NULL-terminate and remove enter */
	buf[count] = '\0';
	strim(buf);

	crashtype = find_crashtype(buf);
	free_page((unsigned long)buf);

	if (!crashtype)
		return -EINVAL;
	ssize_t out;
	char *buf;

	buf = (char *)__get_free_page(GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;


	out = simple_read_from_buffer(user_buf, count, off,
				      buf, n);
	free_page((unsigned long) buf);

	return out;
}
	if (count < 1)
		return -EINVAL;

	buf = (char *)__get_free_page(GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	if (copy_from_user(buf, user_buf, count)) {
		free_page((unsigned long) buf);
		return -EFAULT;
	}
	/* NULL-terminate and remove enter */
	buf[count] = '\0';
	strim(buf);

	crashtype = find_crashtype(buf);
	free_page((unsigned long) buf);
	if (!crashtype)
		return -EINVAL;
