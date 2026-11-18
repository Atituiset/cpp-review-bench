// AUTO-DRAFT from torvalds/linux PR #63fdb511c27afcebce797e3db580b3534ddfa639
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define TEMP_MEMORY_LIMIT 65536
/* …（同文件无关代码省略）… */
static void *temp_kmalloc(size_t *size, void *backup, size_t backup_len)
{
	void *res;

	if (WARN_ON(*size == 0))
		return NULL;

	if (*size < backup_len)
		return backup;

	if (!backup && iommufd_should_fail())
		return NULL;

	*size = min_t(size_t, *size, TEMP_MEMORY_LIMIT);
	res = kmalloc(*size, GFP_KERNEL | __GFP_NOWARN | __GFP_NORETRY);
	if (res)
		return res;
	*size = PAGE_SIZE;
	if (backup_len) {
		res = kmalloc(*size, GFP_KERNEL | __GFP_NOWARN | __GFP_NORETRY);
		if (res)
			return res;
		*size = backup_len;
		return backup;
	}
	return kmalloc(*size, GFP_KERNEL);
}
/* …（同文件无关代码省略）… */
void interval_tree_double_span_iter_update(
	struct interval_tree_double_span_iter *iter)
{
	unsigned long last_hole = ULONG_MAX;
	unsigned int i;

	for (i = 0; i != ARRAY_SIZE(iter->spans); i++) {
		if (interval_tree_span_iter_done(&iter->spans[i])) {
			iter->is_used = -1;
			return;
		}

		if (iter->spans[i].is_hole) {
			last_hole = min(last_hole, iter->spans[i].last_hole);
			continue;
		}

		iter->is_used = i + 1;
		iter->start_used = iter->spans[i].start_used;
		iter->last_used = min(iter->spans[i].last_used, last_hole);
		return;
	}

	iter->is_used = 0;
	iter->start_hole = iter->spans[0].start_hole;
	iter->last_hole =
		min(iter->spans[0].last_hole, iter->spans[1].last_hole);
}
/* …（同文件无关代码省略）… */
void interval_tree_double_span_iter_next(
	struct interval_tree_double_span_iter *iter)
{
	unsigned int i;

	if (iter->is_used == -1 ||
	    iter->last_hole == iter->spans[0].last_index) {
		iter->is_used = -1;
		return;
	}

	for (i = 0; i != ARRAY_SIZE(iter->spans); i++)
		interval_tree_span_iter_advance(
			&iter->spans[i], iter->itrees[i], iter->last_hole + 1);
	interval_tree_double_span_iter_update(iter);
}
/* …（同文件无关代码省略）… */
static void iopt_pages_add_npinned(struct iopt_pages *pages, size_t npages)
{
	int rc;

	rc = check_add_overflow(pages->npinned, npages, &pages->npinned);
	if (IS_ENABLED(CONFIG_IOMMUFD_TEST))
		WARN_ON(rc || pages->npinned > pages->npages);
}
/* …（同文件无关代码省略）… */
static unsigned long iopt_area_index_to_iova(struct iopt_area *area,
					     unsigned long index)
{
	if (IS_ENABLED(CONFIG_IOMMUFD_TEST))
		WARN_ON(index < iopt_area_index(area) ||
			index > iopt_area_last_index(area));
	index -= iopt_area_index(area);
	if (index == 0)
		return iopt_area_iova(area);
	return iopt_area_iova(area) - area->page_offset + index * PAGE_SIZE;
}
/* …（同文件无关代码省略）… */
static struct iopt_area *iopt_pages_find_domain_area(struct iopt_pages *pages,
						     unsigned long index)
{
	struct interval_tree_node *node;

	node = interval_tree_iter_first(&pages->domains_itree, index, index);
	if (!node)
		return NULL;
	return container_of(node, struct iopt_area, pages_node);
}
/* …（同文件无关代码省略）… */
enum batch_kind {
	BATCH_CPU_MEMORY = 0,
	BATCH_MMIO,
};
/* …（同文件无关代码省略）… */
struct pfn_batch {
	unsigned long *pfns;
	u32 *npfns;
	unsigned int array_size;
	unsigned int end;
	unsigned int total_pfns;
	enum batch_kind kind;
};
/* …（同文件无关代码省略）… */
static bool batch_add_pfn_num(struct pfn_batch *batch, unsigned long pfn,
			      u32 nr, enum batch_kind kind)
{
	unsigned int end = batch->end;

	if (batch->kind != kind) {
		/* One kind per batch */
		if (batch->end != 0)
			return false;
		batch->kind = kind;
	}

	if (end && pfn == batch->pfns[end - 1] + batch->npfns[end - 1] &&
	    nr <= MAX_NPFNS - batch->npfns[end - 1]) {
		batch->npfns[end - 1] += nr;
	} else if (end < batch->array_size) {
		batch->pfns[end] = pfn;
		batch->npfns[end] = nr;
		batch->end++;
	} else {
		return false;
	}

	batch->total_pfns += nr;
	return true;
}
/* …（同文件无关代码省略）… */
static void batch_remove_pfn_num(struct pfn_batch *batch, unsigned long nr)
{
	batch->npfns[batch->end - 1] -= nr;
	if (batch->npfns[batch->end - 1] == 0)
		batch->end--;
	batch->total_pfns -= nr;
}
/* …（同文件无关代码省略）… */
static bool batch_add_pfn(struct pfn_batch *batch, unsigned long pfn)
{
	return batch_add_pfn_num(batch, pfn, 1, BATCH_CPU_MEMORY);
}
/* …（同文件无关代码省略）… */
static void batch_from_domain(struct pfn_batch *batch,
			      struct iommu_domain *domain,
			      struct iopt_area *area, unsigned long start_index,
			      unsigned long last_index)
{
	unsigned int page_offset = 0;
	unsigned long iova;
	phys_addr_t phys;

	iova = iopt_area_index_to_iova(area, start_index);
	if (start_index == iopt_area_index(area))
		page_offset = area->page_offset;
	while (start_index <= last_index) {
		/*
		 * This is pretty slow, it would be nice to get the page size
		 * back from the driver, or have the driver directly fill the
		 * batch.
		 */
		phys = iommu_iova_to_phys(domain, iova) - page_offset;
		if (!batch_add_pfn(batch, PHYS_PFN(phys)))
			return;
		iova += PAGE_SIZE - page_offset;
		page_offset = 0;
		start_index++;
	}
}
/* …（同文件无关代码省略）… */
static void batch_from_xarray(struct pfn_batch *batch, struct xarray *xa,
			      unsigned long start_index,
			      unsigned long last_index)
{
	XA_STATE(xas, xa, start_index);
	void *entry;

	rcu_read_lock();
	while (true) {
		entry = xas_next(&xas);
		if (xas_retry(&xas, entry))
			continue;
		WARN_ON(!xa_is_value(entry));
		if (!batch_add_pfn(batch, xa_to_value(entry)) ||
		    start_index == last_index)
			break;
		start_index++;
	}
	rcu_read_unlock();
}
/* …（同文件无关代码省略）… */
static void batch_from_pages(struct pfn_batch *batch, struct page **pages,
			     size_t npages)
{
	struct page **end = pages + npages;

	for (; pages != end; pages++)
		if (!batch_add_pfn(batch, page_to_pfn(*pages)))
			break;
}
/* …（同文件无关代码省略）… */
static int batch_from_folios(struct pfn_batch *batch, struct folio ***folios_p,
			     unsigned long *offset_p, unsigned long npages)
{
	int rc = 0;
	struct folio **folios = *folios_p;
	unsigned long offset = *offset_p;

	while (npages) {
		struct folio *folio = *folios;
		unsigned long nr = folio_nr_pages(folio) - offset;
		unsigned long pfn = page_to_pfn(folio_page(folio, offset));

		nr = min(nr, npages);
		npages -= nr;

		if (!batch_add_pfn_num(batch, pfn, nr, BATCH_CPU_MEMORY))
			break;
		if (nr > 1) {
			rc = folio_add_pins(folio, nr - 1);
			if (rc) {
				batch_remove_pfn_num(batch, nr);
				goto out;
			}
		}

		folios++;
		offset = 0;
	}

out:
	*folios_p = folios;
	*offset_p = offset;
	return rc;
}
/* …（同文件无关代码省略）… */
struct pfn_reader_user {
	struct page **upages;
	size_t upages_len;
	unsigned long upages_start;
	unsigned long upages_end;
	unsigned int gup_flags;
	/*
	 * 1 means mmget() and mmap_read_lock(), 0 means only mmget(), -1 is
	 * neither
	 */
	int locked;

	/* The following are only valid if file != NULL. */
	struct file *file;
	struct folio **ufolios;
	size_t ufolios_len;
	unsigned long ufolios_offset;
	struct folio **ufolios_next;
};
/* …（同文件无关代码省略）… */
	user->file = (pages->type == IOPT_ADDRESS_FILE) ? pages->file : NULL;
	user->ufolios = NULL;
	user->ufolios_len = 0;
	user->ufolios_next = NULL;
	user->ufolios_offset = 0;
}
/* …（同文件无关代码省略）… */
static long pin_memfd_pages(struct pfn_reader_user *user, unsigned long start,
			    unsigned long npages)
{
	unsigned long i;
	unsigned long offset;
	unsigned long npages_out = 0;
	struct page **upages = user->upages;
	unsigned long end = start + (npages << PAGE_SHIFT) - 1;
	long nfolios = user->ufolios_len / sizeof(*user->ufolios);

	/*
	 * todo: memfd_pin_folios should return the last pinned offset so
	 * we can compute npages pinned, and avoid looping over folios here
	 * if upages == NULL.
	 */
	nfolios = memfd_pin_folios(user->file, start, end, user->ufolios,
				   nfolios, &offset);
	if (nfolios <= 0)
		return nfolios;

	offset >>= PAGE_SHIFT;
	user->ufolios_next = user->ufolios;
	user->ufolios_offset = offset;

	for (i = 0; i < nfolios; i++) {
		struct folio *folio = user->ufolios[i];
		unsigned long nr = folio_nr_pages(folio);
		unsigned long npin = min(nr - offset, npages);

		npages -= npin;
		npages_out += npin;

		if (upages) {
			if (npin == 1) {
				*upages++ = folio_page(folio, offset);
			} else {
				int rc = folio_add_pins(folio, npin - 1);

				if (rc)
					return rc;

				while (npin--)
					*upages++ = folio_page(folio, offset++);
			}
		}

		offset = 0;
	}

	return npages_out;
}
/* …（同文件无关代码省略）… */
static int pfn_reader_user_pin(struct pfn_reader_user *user,
			       struct iopt_pages *pages,
			       unsigned long start_index,
			       unsigned long last_index)
{
	bool remote_mm = pages->source_mm != current->mm;
	unsigned long npages = last_index - start_index + 1;
	unsigned long start;
	unsigned long unum;
	uintptr_t uptr;
	long rc;

	if (IS_ENABLED(CONFIG_IOMMUFD_TEST) &&
	    WARN_ON(last_index < start_index))
		return -EINVAL;

	if (!user->file && !user->upages) {
		/* All undone in pfn_reader_destroy() */
		user->upages_len = npages * sizeof(*user->upages);
		user->upages = temp_kmalloc(&user->upages_len, NULL, 0);
		if (!user->upages)
			return -ENOMEM;
	}

	if (user->file && !user->ufolios) {
		user->ufolios_len = npages * sizeof(*user->ufolios);
		user->ufolios = temp_kmalloc(&user->ufolios_len, NULL, 0);
		if (!user->ufolios)
			return -ENOMEM;
	}

	if (user->locked == -1) {
		/*
		 * The majority of usages will run the map task within the mm
		 * providing the pages, so we can optimize into
		 * get_user_pages_fast()
		 */
		if (!user->file && remote_mm) {
			if (!mmget_not_zero(pages->source_mm))
				return -EFAULT;
		}
		user->locked = 0;
	}

	unum = user->file ? user->ufolios_len / sizeof(*user->ufolios) :
			    user->upages_len / sizeof(*user->upages);
	npages = min_t(unsigned long, npages, unum);

	if (iommufd_should_fail())
		return -EFAULT;

	if (user->file) {
		start = pages->start + (start_index * PAGE_SIZE);
		rc = pin_memfd_pages(user, start, npages);
	} else if (!remote_mm) {
		uptr = (uintptr_t)(pages->uptr + start_index * PAGE_SIZE);
		rc = pin_user_pages_fast(uptr, npages, user->gup_flags,
					 user->upages);
	} else {
		uptr = (uintptr_t)(pages->uptr + start_index * PAGE_SIZE);
		if (!user->locked) {
			mmap_read_lock(pages->source_mm);
			user->locked = 1;
		}
		rc = pin_user_pages_remote(pages->source_mm, uptr, npages,
					   user->gup_flags, user->upages,
					   &user->locked);
	}
	if (rc <= 0) {
		if (WARN_ON(!rc))
			return -EFAULT;
		return rc;
	}
	iopt_pages_add_npinned(pages, rc);
	user->upages_start = start_index;
	user->upages_end = start_index + rc;
	return 0;
}
/* …（同文件无关代码省略）… */
struct pfn_reader_dmabuf {
	struct phys_vec phys;
	unsigned long start_offset;
};
/* …（同文件无关代码省略）… */
static int pfn_reader_fill_dmabuf(struct pfn_reader_dmabuf *dmabuf,
				  struct pfn_batch *batch,
				  unsigned long start_index,
				  unsigned long last_index)
{
	unsigned long start = dmabuf->start_offset + start_index * PAGE_SIZE;

	/*
	 * start/last_index and start are all PAGE_SIZE aligned, the batch is
	 * always filled using page size aligned PFNs just like the other types.
	 * If the dmabuf has been sliced on a sub page offset then the common
	 * batch to domain code will adjust it before mapping to the domain.
	 */
	batch_add_pfn_num(batch, PHYS_PFN(dmabuf->phys.paddr + start),
			  last_index - start_index + 1, BATCH_MMIO);
	return 0;
}
/* …（同文件无关代码省略）… */
struct pfn_reader {
	struct iopt_pages *pages;
	struct interval_tree_double_span_iter span;
	struct pfn_batch batch;
	unsigned long batch_start_index;
	unsigned long batch_end_index;
	unsigned long last_index;

	union {
		struct pfn_reader_user user;
		struct pfn_reader_dmabuf dmabuf;
	};
};
/* …（同文件无关代码省略）… */
static int pfn_reader_fill_span(struct pfn_reader *pfns)
{
	struct interval_tree_double_span_iter *span = &pfns->span;
	unsigned long start_index = pfns->batch_end_index;
	struct pfn_reader_user *user;
	unsigned long npages;
	struct iopt_area *area;
	int rc;

	if (IS_ENABLED(CONFIG_IOMMUFD_TEST) &&
	    WARN_ON(span->last_used < start_index))
		return -EINVAL;

	/*
	 * Always read a dmabuf from its phys, even where a domain already
	 * maps it: a domain can only report CPU memory PFNs, losing
	 * BATCH_MMIO. last_hole aliases last_used, so this covers any span.
	 */
	if (iopt_is_dmabuf(pfns->pages))
		return pfn_reader_fill_dmabuf(&pfns->dmabuf, &pfns->batch,
					      start_index, span->last_hole);

	if (span->is_used == 1) {
		batch_from_xarray(&pfns->batch, &pfns->pages->pinned_pfns,
				  start_index, span->last_used);
		return 0;
	}

	if (span->is_used == 2) {
		/*
		 * Pull as many pages from the first domain we find in the
		 * target span. If it is too small then we will be called again
		 * and we'll find another area.
		 */
		area = iopt_pages_find_domain_area(pfns->pages, start_index);
		if (WARN_ON(!area))
			return -EINVAL;

		/* The storage_domain cannot change without the pages mutex */
		batch_from_domain(
			&pfns->batch, area->storage_domain, area, start_index,
			min(iopt_area_last_index(area), span->last_used));
		return 0;
	}

	user = &pfns->user;
	if (start_index >= user->upages_end) {
		rc = pfn_reader_user_pin(user, pfns->pages, start_index,
					 span->last_hole);
		if (rc)
			return rc;
	}

	npages = user->upages_end - start_index;
	start_index -= user->upages_start;
	rc = 0;

	if (!user->file)
		batch_from_pages(&pfns->batch, user->upages + start_index,
				 npages);
	else
		rc = batch_from_folios(&pfns->batch, &user->ufolios_next,
				       &user->ufolios_offset, npages);
	return rc;
}
/* …（同文件无关代码省略）… */
			return -EINVAL;

		rc = pfn_reader_fill_span(pfns);
		if (rc)
			return rc;

		if (WARN_ON(!pfns->batch.total_pfns))
			return -EINVAL;

		pfns->batch_end_index =
			pfns->batch_start_index + pfns->batch.total_pfns;
		if (pfns->batch_end_index == pfns->span.last_used + 1)
			interval_tree_double_span_iter_next(&pfns->span);

/* …（同文件无关代码省略）… */
		if (!user->file) {
			unpin_user_pages(user->upages + start_index, npages);
		} else {
			long n = user->ufolios_len / sizeof(*user->ufolios);

			unpin_folios(user->ufolios_next,
				     user->ufolios + n - user->ufolios_next);
