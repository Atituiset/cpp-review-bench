// AUTO-DRAFT from torvalds/linux PR #aacb7797504183a90d66b19b1efb555cfe9b3eec
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
};

enum {
	OVL_REDIRECT_OFF,	/* "off" mode is never used. In effect	*/
	OVL_REDIRECT_FOLLOW,	/* ...it translates to either "follow"	*/
	OVL_REDIRECT_NOFOLLOW,	/* ...or "nofollow".			*/
	OVL_REDIRECT_ON,
};

/* …（同文件无关代码省略）… */
bool ovl_inuse_trylock(struct dentry *dentry);
void ovl_inuse_unlock(struct dentry *dentry);
bool ovl_is_inuse(struct dentry *dentry);
bool ovl_need_index(struct dentry *dentry);
int ovl_nlink_start(struct dentry *dentry);
void ovl_nlink_end(struct dentry *dentry);
/* …（同文件无关代码省略）… */
	return ovl_get_dir_xattr_val(ofs, path, OVL_XATTR_OPAQUE);
}

static inline bool ovl_redirect_follow(struct ovl_fs *ofs)
{
	return ofs->config.redirect_mode != OVL_REDIRECT_NOFOLLOW;
}

static inline bool ovl_redirect_dir(struct ovl_fs *ofs)
{
	return ofs->config.redirect_mode == OVL_REDIRECT_ON;
