// AUTO-DRAFT from torvalds/linux PR #0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#include "debug.h"
#include "ntfs.h"
#include "ntfs_fs.h"

/*
 * You can set external NTFS_MIN_LOG2_OF_CLUMP/NTFS_MAX_LOG2_OF_CLUMP to manage
/* …（同文件无关代码省略）… */
static int attr_load_runs(struct ATTRIB *attr, struct ntfs_inode *ni,
			  struct runs_tree *run, const CLST *vcn)
{
	int err;
	CLST svcn = le64_to_cpu(attr->nres.svcn);
	CLST evcn = le64_to_cpu(attr->nres.evcn);
	u32 asize;
	u16 run_off;

	if (svcn >= evcn + 1 || run_is_mapped_full(run, svcn, evcn))
		return 0;

	if (vcn && (evcn < *vcn || *vcn < svcn))
		return -EINVAL;

	asize = le32_to_cpu(attr->size);
	run_off = le16_to_cpu(attr->nres.run_off);

	if (run_off > asize)
		return -EINVAL;

	err = run_unpack_ex(run, ni->mi.sbi, ni->mi.rno, svcn, evcn,
			    vcn ? *vcn : svcn, Add2Ptr(attr, run_off),
			    asize - run_off);
	if (err < 0)
		return err;

	return 0;
}
/* …（同文件无关代码省略）… */
	CLST flen, vcn0 = vcn, pre = pre_alloc ? *pre_alloc : 0;
	size_t cnt = run->count;

	for (;;) {
		err = ntfs_look_for_free_space(sbi, lcn, len + pre, &lcn, &flen,
					       opt);
/* …（同文件无关代码省略）… */
 */
int attr_make_nonresident(struct ntfs_inode *ni, struct ATTRIB *attr,
			  struct ATTR_LIST_ENTRY *le, struct mft_inode *mi,
			  u64 new_size, struct runs_tree *run,
			  struct ATTRIB **ins_attr, struct page *page)
{
	struct ntfs_sb_info *sbi;
	struct ATTRIB *attr_s;
/* …（同文件无关代码省略）… */
static int attr_set_size_res(struct ntfs_inode *ni, struct ATTRIB *attr,
			     struct ATTR_LIST_ENTRY *le, struct mft_inode *mi,
			     u64 new_size, struct runs_tree *run,
			     struct ATTRIB **ins_attr)
{
	struct ntfs_sb_info *sbi = mi->sbi;
	struct MFT_REC *rec = mi->mrec;
	u32 used = le32_to_cpu(rec->used);
	u32 asize = le32_to_cpu(attr->size);
	u32 aoff = PtrOffset(rec, attr);
	u32 rsize = le32_to_cpu(attr->res.data_size);
	u32 tail = used - aoff - asize;
	char *next = Add2Ptr(attr, asize);
	s64 dsize = ALIGN(new_size, 8) - ALIGN(rsize, 8);

	if (dsize < 0) {
		memmove(next + dsize, next, tail);
	} else if (dsize > 0) {
		if (used + dsize > sbi->max_bytes_per_attr)
			return attr_make_nonresident(ni, attr, le, mi, new_size,
						     run, ins_attr, NULL);

		memmove(next + dsize, next, tail);
		memset(next, 0, dsize);
	}

	if (new_size > rsize)
		memset(Add2Ptr(resident_data(attr), rsize), 0,
		       new_size - rsize);

	rec->used = cpu_to_le32(used + dsize);
	attr->size = cpu_to_le32(asize + dsize);
	attr->res.data_size = cpu_to_le32(new_size);
	mi->dirty = true;
	*ins_attr = attr;

	return 0;
}
/* …（同文件无关代码省略）… */
		goto bad_inode;
	}

	if (!attr_b->non_res) {
		err = attr_set_size_res(ni, attr_b, le_b, mi_b, new_size, run,
					&attr_b);
/* …（同文件无关代码省略）… */
		}

next_le_1:
		svcn = le64_to_cpu(attr->nres.svcn);
		evcn = le64_to_cpu(attr->nres.evcn);
	}
/* …（同文件无关代码省略）… */
	if (res)
		*res = NULL;

	/* Try to find in cache. */
	down_read(&ni->file.run_lock);
	if (run_lookup_entry_da(&ni->file.run, !no_da ? &ni->file.run_da : NULL,
/* …（同文件无关代码省略）… */
	int step;

again:
	if (run_lookup_entry_da(run, da ? &ni->file.run_da : NULL, vcn, lcn,
				len)) {
	} else {
		*len = 0;
	}

	if (*len) {
		if (*lcn != SPARSE_LCN || !new)
/* …（同文件无关代码省略）… */

	if (!attr_b->non_res) {
		u32 data_size = le32_to_cpu(attr_b->res.data_size);
		*lcn = RESIDENT_LCN;
		*len = data_size;
		if (res) {
/* …（同文件无关代码省略）… */
		to_alloc = ((vcn0 + clen + clst_per_frame - 1) & cmask) - vcn;
		if (fr < clst_per_frame)
			fr = clst_per_frame;
		if (vcn != vcn0)
			zero = true;

		/* Check if 'vcn' and 'vcn0' in different attribute segments. */
/* …（同文件无关代码省略）… */
	total_size = total_size0 + ((u64)*len << cluster_bits);

	if (vcn != vcn0) {
		if (!run_lookup_entry(run, vcn0, lcn, len, NULL)) {
			err = -EINVAL;
			goto out;
/* …（同文件无关代码省略）… */
		return -ENOENT;
	}

	svcn = le64_to_cpu(attr->nres.svcn);
	evcn = le64_to_cpu(attr->nres.evcn);

/* …（同文件无关代码省略）… */
	struct ATTR_LIST_ENTRY *le, *le_b;
	struct mft_inode *mi, *mi_b;
	CLST svcn, evcn1, next_svcn, len;
	CLST vcn, end, clst_data;
	u64 total_size, valid_size, data_size;

	le_b = NULL;
/* …（同文件无关代码省略）… */
	svcn = le64_to_cpu(attr_b->nres.svcn);
	evcn1 = le64_to_cpu(attr_b->nres.evcn) + 1;
	data_size = le64_to_cpu(attr_b->nres.data_size);

	if (svcn <= vcn && vcn < evcn1) {
		attr = attr_b;
/* …（同文件无关代码省略）… */
	attr = ni_find_attr(ni, attr, &le, ATTR_DATA, ni->file.ads.name,
			    ni->file.ads.len, &svcn, &mi);

	if (attr) {
		CLST alloc = bytes_to_cluster(
			sbi, le64_to_cpu(attr_b->nres.alloc_size));
		CLST evcn = le64_to_cpu(attr->nres.evcn);

		if (end < next_svcn)
			end = next_svcn;
		while (end > evcn) {
			/* Remove segment [svcn : evcn). */
			mi_remove_attr(NULL, mi, attr);

			if (!al_remove_le(ni, le)) {
				err = -EINVAL;
				goto out;
			}

			if (evcn + 1 >= alloc) {
				/* Last attribute segment. */
				evcn1 = evcn + 1;
				goto ins_ext;
			}

			if (ni_load_mi(ni, le, &mi)) {
				attr = NULL;
				goto out;
			}

			attr = mi_find_attr(ni, mi, NULL, ATTR_DATA,
					    ni->file.ads.name, ni->file.ads.len,
					    &le->id);
			if (!attr) {
				err = -EINVAL;
				goto out;
			}
			svcn = le64_to_cpu(attr->nres.svcn);
			evcn = le64_to_cpu(attr->nres.evcn);
		}

		if (end < svcn)
			end = svcn;

		err = attr_load_runs(attr, ni, run, &end);
		if (err)
			goto out;

		evcn1 = evcn + 1;
		attr->nres.svcn = cpu_to_le64(next_svcn);
		err = mi_pack_runs(mi, attr, run, evcn1 - next_svcn);
		if (err)
			goto out;

		le->vcn = cpu_to_le64(next_svcn);
		ni->attr_list.dirty = true;
		mi->dirty = true;

		next_svcn = le64_to_cpu(attr->nres.evcn) + 1;
	}
ins_ext:
	if (evcn1 > next_svcn) {
		err = ni_insert_nonresident(ni, ATTR_DATA, ni->file.ads.name,
/* …（同文件无关代码省略）… */
					    &attr, &mi, NULL);
		if (err)
			goto out;
	}
ok:
	run_truncate_around(run, vcn);
/* …（同文件无关代码省略）… */
		if (new_valid > data_size)
			new_valid = data_size;

		valid_size = le64_to_cpu(attr_b->nres.valid_size);
		if (new_valid != valid_siz
