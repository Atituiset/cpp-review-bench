// AUTO-DRAFT from torvalds/linux PR #2a49d49159b76f6dd587131b3765d3458cc3aa01
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
#define CACHEFILES_COOKIE_TYPE_DATA 1
/* …（同文件无关代码省略）… */
	__u8	data[];		/* netfs volume coherency data */
} __packed;

/*
 * set the state xattr on a cache file
 */
/* …（同文件无关代码省略）… */
	struct dentry *dentry;
	struct file *file = object->file;
	unsigned int len = object->cookie->aux_len;
	int ret;

	if (!file)
/* …（同文件无关代码省略）… */
	if (!buf)
		return -ENOMEM;

	buf->object_size	= cpu_to_be64(object->cookie->object_size);
	buf->zero_point		= 0;
	buf->type		= CACHEFILES_COOKIE_TYPE_DATA;
	buf->content		= object->content_info;
/* …（同文件无关代码省略）… */
		trace_cachefiles_vfs_error(object, file_inode(file), ret,
					   cachefiles_trace_setxattr_error);
		trace_cachefiles_coherency(object, file_inode(file)->i_ino,
					   buf->data, buf->content,
					   cachefiles_coherency_set_fail);
		if (ret != -ENOMEM)
			cachefiles_io_error_obj(
				object,
				"Failed to set xattr with error %d", ret);
	} else {
		trace_cachefiles_coherency(object, file_inode(file)->i_ino,
					   buf->data, buf->content,
					   cachefiles_coherency_set_ok);
	}

/* …（同文件无关代码省略）… */
{
	struct cachefiles_xattr *buf;
	struct dentry *dentry = file->f_path.dentry;
	unsigned int len = object->cookie->aux_len, tlen;
	const void *p = fscache_get_aux(object->cookie);
	enum cachefiles_coherency_trace why;
	ssize_t xlen;
	int ret = -ESTALE;

	tlen = sizeof(struct cachefiles_xattr) + len;
/* …（同文件无关代码省略）… */
	if (xlen != tlen) {
		if (xlen < 0) {
			ret = xlen;
			trace_cachefiles_vfs_error(object, file_inode(file), xlen,
						   cachefiles_trace_getxattr_error);
		}
		if (xlen == -EIO)
			cachefiles_io_error_obj(
				object,
				"Failed to read aux with error %zd", xlen);
		why = cachefiles_coherency_check_xattr;
		goto out;
	}

	if (buf->type != CACHEFILES_COOKIE_TYPE_DATA) {
		why = cachefiles_coherency_check_type;
	} else if (memcmp(buf->data, p, len) != 0) {
		why = cachefiles_coherency_check_aux;
	} else if (be64_to_cpu(buf->object_size) != object->cookie->object_size) {
		why = cachefiles_coherency_check_objsize;
	} else if (buf->content == CACHEFILES_CONTENT_DIRTY) {
		// TODO: Begin conflict resolution
		pr_warn("Dirty object in cache\n");
		why = cachefiles_coherency_check_dirty;
	} else {
		why = cachefiles_coherency_check_ok;
		ret = 0;
	}

out:
	trace_cachefiles_coherency(object, file_inode(file)->i_ino,
				   buf->data, buf->content, why);
	kfree(buf);
	return ret;
/* …（同文件无关代码省略）… */
{
	int ret;

	ret = cachefiles_inject_remove_error();
	if (ret == 0) {
		ret = mnt_want_write(cache->mnt);
