// AUTO-DRAFT from torvalds/linux PR #0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "debug.h"
#include "ntfs.h"
#include "ntfs_fs.h"

/* Convert little endian UTF-16 to NLS string. */
int ntfs_utf16_to_nls(struct ntfs_sb_info *sbi, const __le16 *name, u32 len,
		      u8 *buf, int buf_len)
{
	int ret, warn;
	u8 *op;
	struct nls_table *nls = sbi->options->nls;

	static_assert(sizeof(wchar_t) == sizeof(__le16));

	if (buf_len <= 0)
		return -EINVAL;

	buf_len -= 1;

	if (!nls) {
		/* UTF-16 -> UTF-8 */
		ret = utf16s_to_utf8s((wchar_t *)name, len, UTF16_LITTLE_ENDIAN,
				      buf, buf_len);
		buf[ret] = '\0';
		return ret;
	}

	op = buf;
	warn = 0;

	while (len--) {
		u16 ec;
		int charlen;
		char dump[5];

		if (buf_len < NLS_MAX_CHARSET_SIZE) {
			ntfs_warn(sbi->sb,
				  "filename was truncated while converting.");
			break;
		}

		ec = le16_to_cpu(*name++);
		charlen = nls->uni2char(ec, op, buf_len);

		if (charlen > 0) {
			op += charlen;
			buf_len -= charlen;
			continue;
		}

		*op++ = '_';
		buf_len -= 1;
		if (warn)
			continue;

		warn = 1;
		hex_byte_pack(&dump[0], ec >> 8);
		hex_byte_pack(&dump[2], ec);
		dump[4] = 0;

		ntfs_err(sbi->sb, "failed to convert \"%s\" to %s", dump,
			 nls->charset);
	}

	*op = '\0';
	return op - buf;
}
/* …（同文件无关代码省略）… */
	struct inode *inode = NULL;
	struct ntfs_fnd *fnd_a = NULL;

	if (!fnd) {
		fnd_a = fnd_get();
		if (!fnd_a) {
/* …（同文件无关代码省略）… */
static inline bool de_fname_fits(const struct NTFS_DE *e, u32 e_size,
				 const struct ATTR_FILE_NAME *fname)
{
	return sizeof(struct NTFS_DE) + fname_full_size(fname) <= e_size;
}
/* …（同文件无关代码省略）… */
static inline bool ntfs_dir_emit(struct ntfs_sb_info *sbi,
				 struct ntfs_inode *ni, const struct NTFS_DE *e,
				 u8 *name, struct dir_context *ctx)
{
	const struct ATTR_FILE_NAME *fname;
	u64 ino;
	int name_len;
	u32 dt_type;

	fname = Add2Ptr(e, sizeof(struct NTFS_DE));

	if (fname->type == FILE_NAME_DOS)
		return true;

	if (!mi_is_ref(&ni->mi, &fname->home))
		return true;

	ino = ino_get(&e->ref);

	if (ino == MFT_REC_ROOT)
		return true;

	/* Skip meta files. Unless option to show metafiles is set. */
	if (!sbi->options->showmeta && ntfs_is_meta_file(sbi, ino))
		return true;

	if (sbi->options->nohidden && (fname->dup.fa & FILE_ATTRIBUTE_HIDDEN))
		return true;

	if (!de_fname_fits(e, le16_to_cpu(e->size), fname))
		return true;

	name_len = ntfs_utf16_to_nls(sbi, fname->name, fname->name_len, name,
				     PATH_MAX);
	if (name_len <= 0) {
		ntfs_warn(sbi->sb, "failed to convert name for inode %llx.",
			  ino);
		return true;
	}

	/*
	 * NTFS: symlinks are "dir + reparse" or "file + reparse"
	 * Unfortunately reparse attribute is used for many purposes (several dozens).
	 * It is not possible here to know is this name symlink or not.
	 * To get exactly the type of name we should to open inode (read mft).
	 * getattr for opened file (fstat) correctly returns symlink.
	 */
	dt_type = (fname->dup.fa & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;

	/*
	 * It is not reliable to detect the type of name using duplicated information
	 * stored in parent directory.
	 * The only correct way to get the type of name - read MFT record and find ATTR_STD.
	 * The code below is not good idea.
	 * It does additional locks/reads just to get the type of name.
	 * Should we use additional mount option to enable branch below?
	 */
	if (fname->dup.extend_data && ino != ni->mi.rno) {
		struct inode *inode = ntfs_iget5(sbi->sb, &e->ref, NULL);
		if (!IS_ERR_OR_NULL(inode)) {
			dt_type = fs_umode_to_dtype(inode->i_mode);
			iput(inode);
		}
	}

	return dir_emit(ctx, (s8 *)name, name_len, ino, dt_type);
}
/* …（同文件无关代码省略）… */
static int ntfs_read_hdr(struct ntfs_sb_info *sbi, struct ntfs_inode *ni,
			 const struct INDEX_HDR *hdr, u64 vbo, u64 pos,
			 u8 *name, struct dir_context *ctx)
{
	const struct NTFS_DE *e;
	u32 e_size;
	u32 end = le32_to_cpu(hdr->used);
	u32 off = le32_to_cpu(hdr->de_off);

	for (;; off += e_size) {
		if (off + sizeof(struct NTFS_DE) > end)
			return -EINVAL;

		e = Add2Ptr(hdr, off);
		e_size = le16_to_cpu(e->size);
		if (e_size < sizeof(struct NTFS_DE) || off + e_size > end)
			return -EINVAL;

		if (de_is_last(e))
			return 0;

		/* Skip already enumerated. */
		if (vbo + off < pos)
			continue;

		if (le16_to_cpu(e->key_size) < SIZEOF_ATTRIBUTE_FILENAME)
			return -EINVAL;

		ctx->pos = vbo + off;

		/* Submit the name to the filldir callback. */
		if (!ntfs_dir_emit(sbi, ni, e, name, ctx)) {
			/* ctx is full. */
			return +1;
		}
	}
}
/* …（同文件无关代码省略）… */
	if (!name)
		return -ENOMEM;

	if (!ni->mi_loaded && ni->attr_list.size) {
		/*
		 * Directory inode is locked for read.
		 * Load all subrecords to avoid 'write' access to 'ni' during
		 * directory reading.
		 */
		ni_lock(ni);
		if (!ni->mi_loaded && ni->attr_list.size) {
			err = ni_load_all_mi(ni);
			if (!err)
				ni->mi_loaded = true;
		}
		ni_unlock(ni);
		if (err)
			goto out;
	}

	/*
	 * Keep directory metadata stable for the whole walk. Loading subrecords
	 * once is not enough if concurrent writeback can still compact ATTR_LIST
	 * entries and free the record that ntfs_read_hdr() is currently walking.
	 */
	ni_lock(ni);

	root = indx_get_root(&ni->dir, ni, NULL, NULL);
	if (!root) {
		err = -EINVAL;
		goto out_unlock;
	}

	if (pos >= sbi->record_size) {
		bit = (pos - sbi->record_size) >> index_bits;
	} else {
		/*
		 * Add each name from root in 'ctx'.
		 */
		err = ntfs_read_hdr(sbi, ni, &root->ihdr, 0, pos, name, ctx);
		if (err)
			goto out_unlock;
		bit = 0;
	}

/* …（同文件无关代码省略）… */
		/* Get the next used index. */
		err = indx_used_bit(&ni->dir, ni, &bit);
		if (err)
			goto out_unlock;

		if (bit == MINUS_ONE_T) {
			/* no more used indexes. end of dir. */
/* …（同文件无关代码省略）… */
		if (bit >= max_bit) {
			/* Corrupted directory. */
			err = -EINVAL;
			goto out_unlock;
		}

		err = indx_read_ra(&ni->dir, ni, bit << ni->dir.idx2vbn_bits,
				   &node, &file->f_ra);
		if (err)
			goto out_unlock;

	
