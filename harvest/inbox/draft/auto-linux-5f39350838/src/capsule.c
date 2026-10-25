// AUTO-DRAFT from torvalds/linux PR #89d2d7bf0698b7fdfc914bfd219e739e9308d332
typedef struct {
	u64 length;
	u64 data;
} efi_capsule_block_desc_t;
/* …（同文件无关代码省略）… */
#define SGLIST_PER_PAGE	((PAGE_SIZE / sizeof(efi_capsule_block_desc_t)) - 1)
/* …（同文件无关代码省略）… */
	for (i = 0; i < sg_count; i++) {
		efi_capsule_block_desc_t *sglist;
  // <<< BUG ANCHOR
		sglist = kmap_atomic(sg_pages[i]);

		for (j = 0; j < SGLIST_PER_PAGE && count > 0; j++) {
			u64 sz = min_t(u64, imagesize,
/* …（同文件无关代码省略）… */
		 */
		efi_capsule_flush_cache_range(sglist, PAGE_SIZE);
#endif
		kunmap_atomic(sglist);
	}

	mutex_lock(&capsule_mutex);
