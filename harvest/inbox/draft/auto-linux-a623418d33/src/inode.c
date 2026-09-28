// AUTO-DRAFT from torvalds/linux PR #d266640c6c760c9bc215bf5a3ece122ca488b6f5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	if (!debugfs_enabled)
		return ERR_PTR(-EPERM);

	if (!debugfs_initialized()) {
		pr_err("Unable to create file '%s', debugfs is not initialized yet\n",
		       name);
/* …（同文件无关代码省略）… */

	pr_debug("creating file '%s'\n", name);

	if (IS_ERR(parent))
		return parent;

	error = simple_pin_fs(&debug_fs_type, &debugfs_mount,
			      &debugfs_mount_count);
	if (error) {
/* …（同文件无关代码省略）… */
bool debugfs_initialized(void)
{
	return debugfs_registered;
}
