// AUTO-DRAFT from torvalds/linux PR #0d508d4709837bcc88e314c0533a42d3c55b31e3
#include <linux/namei.h>
#include <linux/file.h>
#include <uapi/linux/mount.h>
#include "internal.h"
#include "mount.h"

/* …（同文件无关代码省略）… */
	if (IS_ERR(fs_name))
		return PTR_ERR(fs_name);

	fs_type = get_fs_type(fs_name);
	kfree(fs_name);
	if (!fs_type)
