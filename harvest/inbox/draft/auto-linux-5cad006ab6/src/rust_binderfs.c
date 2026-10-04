// AUTO-DRAFT from torvalds/linux PR #25d576ed11108470d0999054265108400db1b881
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

	bool oneway_spam_detection;
	bool extended_error;
	bool freeze_notification;
};

static const struct constant_table binderfs_param_stats[] = {
/* …（同文件无关代码省略）… */
	.oneway_spam_detection = true,
	.extended_error = true,
	.freeze_notification = true,
};

static inline struct binderfs_info *BINDERFS_SB(const struct super_block *sb)
{
	return sb->s_fs_info;
}
/* …（同文件无关代码省略）… */
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

	return 0;
}
