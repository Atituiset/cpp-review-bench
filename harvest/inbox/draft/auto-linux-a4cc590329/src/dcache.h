// AUTO-DRAFT from torvalds/linux PR #51628610f9187c1c1cfe499d92e066bc22ad9693
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
 #define HASH_LEN_DECLARE u32 hash; u32 len
/* …（同文件无关代码省略）… */
struct qstr {
	union {
		struct {
			HASH_LEN_DECLARE;
		};
		u64 hash_len;
	};
	const unsigned char *name;
};
/* …（同文件无关代码省略）… */
# define DNAME_INLINE_WORDS 5 /* 192 bytes */
/* …（同文件无关代码省略）… */
#define DNAME_INLINE_LEN (DNAME_INLINE_WORDS*sizeof(unsigned long))
/* …（同文件无关代码省略）… */
union shortname_store {
	unsigned char string[DNAME_INLINE_LEN];
	unsigned long words[DNAME_INLINE_WORDS];
};
/* …（同文件无关代码省略）… */
struct name_snapshot {
	struct qstr name;
	union shortname_store inline_name;
};
void take_dentry_name_snapshot(struct name_snapshot *, struct dentry *);
bool try_take_dentry_name_snapshot(struct name_snapshot *, struct dentry *);
void release_dentry_name_snapshot(struct name_snapshot *);

static inline struct dentry *d_first_child(const struct dentry *dentry)
{
	return hlist_entry_safe(dentry->d_children.first, struct dentry, d_sib);
}
