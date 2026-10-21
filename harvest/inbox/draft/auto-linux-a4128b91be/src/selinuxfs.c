// AUTO-DRAFT from torvalds/linux PR #331eadd85f14bac85663c14c99228f6dedf64d87
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct selinux_fs_info {
	struct dentry *bool_dir;
	unsigned int bool_num;
	char **bool_pending_names;
	int *bool_pending_values;
	struct dentry *class_dir;
	unsigned long last_class_ino;
	unsigned long last_ino;
	struct super_block *sb;
};

/* …（同文件无关代码省略）… */
#define SEL_CLASS_INO_OFFSET		0x04000000
/* …（同文件无关代码省略）… */
#define SEL_INO_MASK			0x00ffffff
/* …（同文件无关代码省略）… */
#define BOOL_DIR_NAME "booleans"
/* …（同文件无关代码省略）… */
#define TMPBUFLEN	12
/* …（同文件无关代码省略）… */
	/* only allows one page from the head */
	if (vma->vm_pgoff > 0 || size != PAGE_SIZE)
		return -EIO;
	/* disallow writable mapping */
	if (vma->vm_flags & VM_WRITE)
		return -EPERM;
	/* disallow mprotect() turns it into writable */
	vm_flags_clear(vma, VM_MAYWRITE);

	return remap_pfn_range(vma, vma->vm_start,
			       page_to_pfn(status),
/* …（同文件无关代码省略）… */
			  int **bool_pending_values);
static int sel_make_classes(struct selinux_policy *newpolicy,
			    struct dentry *class_dir,
			    unsigned long *last_class_ino);

/* declaration for sel_make_class_dirs */
static struct dentry *sel_make_dir(struct dentry *dir, const char *name,
			unsigned long *ino);

/* declaration for sel_make_policy_nodes */
static struct dentry *sel_make_swapover_dir(struct super_block *sb,
						unsigned long *ino);

static ssize_t sel_read_mls(struct file *filp, char __user *buf,
				size_t count, loff_t *ppos)
{
	char tmpbuf[TMPBUFLEN];
	ssize_t length;

	length = scnprintf(tmpbuf, TMPBUFLEN, "%d",
			   security_mls_enabled());
	return simple_read_from_buffer(buf, count, ppos, tmpbuf, length);
}
/* …（同文件无关代码省略）… */
	unsigned int bool_num = 0;
	char **bool_names = NULL;
	int *bool_values = NULL;
	unsigned long tmp_ino = fsi->last_ino; /* Don't increment last_ino in this function */

	tmp_parent = sel_make_swapover_dir(fsi->sb, &tmp_ino);
	if (IS_ERR(tmp_parent))
/* …（同文件无关代码省略）… */
	return err;
}

static inline unsigned long sel_class_to_ino(u16 class)
{
	return (class * (SEL_VEC_MAX + 1)) | SEL_CLASS_INO_OFFSET;
}

static inline u16 sel_ino_to_class(unsigned long ino)
{
	return (ino & SEL_INO_MASK) / (SEL_VEC_MAX + 1);
}

static inline unsigned long sel_perm_to_ino(u16 class, u32 perm)
{
	return (class * (SEL_VEC_MAX + 1) + perm) | SEL_CLASS_INO_OFFSET;
}

static inline u32 sel_ino_to_perm(unsigned long ino)
{
	return (ino & SEL_INO_MASK) % (SEL_VEC_MAX + 1);
}

static ssize_t sel_read_class(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	unsigned long ino = file_inode(file)->i_ino;
	char res[TMPBUFLEN];
	ssize_t len = scnprintf(res, sizeof(res), "%d", sel_ino_to_class(ino));
	return simple_read_from_buffer(buf, count, ppos, res, len);
/* …（同文件无关代码省略）… */
static ssize_t sel_read_perm(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	unsigned long ino = file_inode(file)->i_ino;
	char res[TMPBUFLEN];
	ssize_t len = scnprintf(res, sizeof(res), "%d", sel_ino_to_perm(ino));
	return simple_read_from_buffer(buf, count, ppos, res, len);
/* …（同文件无关代码省略）… */
	int value;
	char tmpbuf[TMPBUFLEN];
	ssize_t length;
	unsigned long i_ino = file_inode(file)->i_ino;

	value = security_policycap_supported(i_ino & SEL_INO_MASK);
	length = scnprintf(tmpbuf, TMPBUFLEN, "%d", value);
/* …（同文件无关代码省略）… */

static int sel_make_classes(struct selinux_policy *newpolicy,
			    struct dentry *class_dir,
			    unsigned long *last_class_ino)
{
	u32 i, nclasses;
	int rc;
/* …（同文件无关代码省略）… */
}

static struct dentry *sel_make_dir(struct dentry *dir, const char *name,
			unsigned long *ino)
{
	struct inode *inode;

/* …（同文件无关代码省略）… */
static int reject_all(const struct mnt_idmap *idmap, struct inode *inode, int mask)
{
	return -EPERM;	// no access for anyone, root or no root.
}
/* …（同文件无关代码省略）… */
	.permission	= reject_all,
};

static struct dentry *sel_make_swapover_dir(struct super_block *sb,
						unsigned long *ino)
{
	struct dentry *dentry;
	struct inode *inode;
/* …（同文件无关代码省略）… */

#define NULL_FILE_NAME "null"

static int sel_fill_super(struct super_block *sb, struct fs_context *fc)
{
	struct selinux_fs_info *fsi;
/* …（同文件无关代码省略）… */
	if (ret)
		goto err;

	fsi = sb->s_fs_info;
	fsi->bool_dir = sel_make_dir(sb->s_root, BOOL_DIR_NAME, &fsi->last_ino);
	if (IS_ERR(fsi->bool_dir)) {
