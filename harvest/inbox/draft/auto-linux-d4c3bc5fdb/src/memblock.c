// AUTO-DRAFT from torvalds/linux PR #394023fbe34fccec5d4db2576169d21e14c8d3d4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define kho_scratch_only false
/* …（同文件无关代码省略）… */
 */
static __refdata struct memblock_type *memblock_memory = &memblock.memory;

#define for_each_memblock_type(i, memblock_type, rgn)			\
	for (i = 0, rgn = &memblock_type->regions[0];			\
	     i < memblock_type->cnt;					\
	     i++, rgn = &memblock_type->regions[i])

#define memblock_dbg(fmt, ...)						\
	do {								\
		if (memblock_debug)					\
/* …（同文件无关代码省略）… */
static enum memblock_flags __init_memblock choose_memblock_flags(void)
{
	/* skip non-scratch memory for kho early boot allocations */
	if (kho_scratch_only)
		return MEMBLOCK_KHO_SCRATCH;

	return system_has_some_mirror ? MEMBLOCK_MIRROR : MEMBLOCK_NONE;
}
/* …（同文件无关代码省略）… */
static inline phys_addr_t memblock_cap_size(phys_addr_t base, phys_addr_t *size)
{
	return *size = min(*size, PHYS_ADDR_MAX - base);
}
/* …（同文件无关代码省略）… */
static phys_addr_t __init_memblock
__memblock_find_range_bottom_up(phys_addr_t start, phys_addr_t end,
				phys_addr_t size, phys_addr_t align, int nid,
				enum memblock_flags flags)
{
	phys_addr_t this_start, this_end, cand;
	u64 i;

	for_each_free_mem_range(i, nid, flags, &this_start, &this_end, NULL) {
		this_start = clamp(this_start, start, end);
		this_end = clamp(this_end, start, end);

		cand = round_up(this_start, align);
		if (cand < this_end && this_end - cand >= size)
			return cand;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static phys_addr_t __init_memblock
__memblock_find_range_top_down(phys_addr_t start, phys_addr_t end,
			       phys_addr_t size, phys_addr_t align, int nid,
			       enum memblock_flags flags)
{
	phys_addr_t this_start, this_end, cand;
	u64 i;

	for_each_free_mem_range_reverse(i, nid, flags, &this_start, &this_end,
					NULL) {
		this_start = clamp(this_start, start, end);
		this_end = clamp(this_end, start, end);

		if (this_end < size)
			continue;

		cand = round_down(this_end - size, align);
		if (cand >= this_start)
			return cand;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static phys_addr_t __init_memblock memblock_find_in_range_node(phys_addr_t size,
					phys_addr_t align, phys_addr_t start,
					phys_addr_t end, int nid,
					enum memblock_flags flags)
{
	/* pump up @end */
	if (end == MEMBLOCK_ALLOC_ACCESSIBLE ||
	    end == MEMBLOCK_ALLOC_NOLEAKTRACE)
		end = memblock.current_limit;

	/* avoid allocating the first page */
	start = max_t(phys_addr_t, start, PAGE_SIZE);
	end = max(start, end);

	if (memblock_bottom_up())
		return __memblock_find_range_bottom_up(start, end, size, align,
						       nid, flags);
	else
		return __memblock_find_range_top_down(start, end, size, align,
						      nid, flags);
}
/* …（同文件无关代码省略）… */
static phys_addr_t __init_memblock memblock_find_in_range(phys_addr_t start,
					phys_addr_t end, phys_addr_t size,
					phys_addr_t align)
{
	phys_addr_t ret;
	enum memblock_flags flags = choose_memblock_flags();

again:
	ret = memblock_find_in_range_node(size, align, start, end,
					    NUMA_NO_NODE, flags);

	if (!ret && (flags & MEMBLOCK_MIRROR)) {
		pr_warn_ratelimited("Could not allocate %pap bytes of mirrored memory\n",
			&size);
		flags &= ~MEMBLOCK_MIRROR;
		goto again;
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static void __init_memblock memblock_remove_region(struct memblock_type *type, unsigned long r)
{
	type->total_size -= type->regions[r].size;
	memmove(&type->regions[r], &type->regions[r + 1],
		(type->cnt - (r + 1)) * sizeof(type->regions[r]));
	type->cnt--;

	/* Special case for empty arrays */
	if (type->cnt == 0) {
		WARN_ON(type->total_size != 0);
		type->regions[0].base = 0;
		type->regions[0].size = 0;
		type->regions[0].flags = 0;
		memblock_set_region_node(&type->regions[0], MAX_NUMNODES);
	}
}
/* …（同文件无关代码省略）… */
static int __init_memblock memblock_double_array(struct memblock_type *type,
						phys_addr_t new_area_start,
						phys_addr_t new_area_size)
{
	struct memblock_region *new_array, *old_array;
	phys_addr_t old_alloc_size, new_alloc_size;
	phys_addr_t old_size, new_size, addr, new_end;
	int use_slab = slab_is_available();
	int *in_slab;

	/* We don't allow resizing until we know about the reserved regions
	 * of memory that aren't suitable for allocation
	 */
	if (!memblock_can_resize)
		panic("memblock: cannot resize %s array\n", type->name);

	/* Calculate new doubled size */
	old_size = type->max * sizeof(struct memblock_region);
	new_size = old_size << 1;
	/*
	 * We need to allocated new one align to PAGE_SIZE,
	 *   so we can free them completely later.
	 */
	old_alloc_size = PAGE_ALIGN(old_size);
	new_alloc_size = PAGE_ALIGN(new_size);

	/* Retrieve the slab flag */
	if (type == &memblock.memory)
		in_slab = &memblock_memory_in_slab;
	else
		in_slab = &memblock_reserved_in_slab;

	/* Try to find some space for it */
	if (use_slab) {
		new_array = kmalloc(new_size, GFP_KERNEL);
		addr = new_array ? __pa(new_array) : 0;
	} else {
		/* only exclude range when trying to double reserved.regions */
		if (type != &memblock.reserved)
			new_area_start = new_area_size = 0;

		addr = memblock_find_in_range(new_area_start + new_area_size,
						memblock.current_limit,
						new_alloc_size, PAGE_SIZE);
		if (!addr && new_area_size)
			addr = memblock_find_in_range(0,
				min(new_area_start, memblock.current_limit),
				new_alloc_size, PAGE_SIZE);

		if (addr) {
			/* The memory may not have been accepted, yet. */
			accept_memory(addr, new_alloc_size);

			new_array = __va(addr);
		} else {
			new_array = NULL;
		}
	}
	if (!addr) {
		pr_err("memblock: Failed to double %s array from %ld to %ld entries !\n",
		       type->name, type->max, type->max * 2);
		return -1;
	}

	new_end = addr + new_size - 1;
	memblock_dbg("memblock: %s is doubled to %ld at [%pa-%pa]",
			type->name, type->max * 2, &addr, &new_end);

	/*
	 * Found space, we now need to move the array over before we add the
	 * reserved region since it may be our reserved array itself that is
	 * full.
	 */
	memcpy(new_array, type->regions, old_size);
	memset(new_array + type->max, 0, old_size);
	old_array = type->regions;
	type->regions = new_array;
	type->max <<= 1;

	/* Free old array. We needn't free it if the array is the static one */
	if (*in_slab)
		kfree(old_array);
	else if (old_array != memblock_memory_init_regions &&
		 old_array != memblock_reserved_init_regions)
		memblock_free(old_array, old_alloc_size);

	/*
	 * Reserve the new array if that comes from the memblock.  Otherwise, we
	 * needn't do it
	 */
	if (!use_slab)
		BUG_ON(memblock_reserve_kern(addr, new_alloc_size));

	/* Update slab flag */
	*in_slab = use_slab;

	return 0;
}
/* …（同文件无关代码省略）… */
static void __init_memblock memblock_merge_regions(struct memblock_type *type,
						   unsigned long start_rgn,
						   unsigned long end_rgn)
{
	int i = 0;
	if (start_rgn)
		i = start_rgn - 1;
	end_rgn = min(end_rgn, type->cnt - 1);
	while (i < end_rgn) {
		struct memblock_region *this = &type->regions[i];
		struct memblock_region *next = &type->regions[i + 1];

		if (this->base + this->size != next->base ||
		    memblock_get_region_node(this) !=
		    memblock_get_region_node(next) ||
		    this->flags != next->flags) {
			BUG_ON(this->base + this->size > next->base);
			i++;
			continue;
		}

		this->size += next->size;
		/* move forward from next + 1, index of which is i + 2 */
		memmove(next, next + 1, (type->cnt - (i + 2)) * sizeof(*next));
		type->cnt--;
		end_rgn--;
	}
}
/* …（同文件无关代码省略）… */
static void __init_memblock memblock_insert_region(struct memblock_type *type,
						   int idx, phys_addr_t base,
						   phys_addr_t size,
						   int nid,
						   enum memblock_flags flags)
{
	struct memblock_region *rgn = &type->regions[idx];

	BUG_ON(type->cnt >= type->max);
	memmove(rgn + 1, rgn, (type->cnt - idx) * sizeof(*rgn));
	rgn->base = base;
	rgn->size = size;
	rgn->flags = flags;
	memblock_set_region_node(rgn, nid);
	type->cnt++;
	type->total_size += size;
}

/**
 * memblock_add_range - add new memblock region
 * @type: memblock type to add new region into
/* …（同文件无关代码省略）… */
static int __init_memblock memblock_add_range(struct memblock_type *type,
				phys_addr_t base, phys_addr_t size,
				int nid, enum memblock_flags flags)
{
	bool insert = false;
	phys_addr_t obase = base;
	phys_addr_t end = base + memblock_cap_size(base, &size);
	int idx, nr_new, start_rgn = -1, end_rgn;
	struct memblock_region *rgn;

	if (!size)
		return 0;

	/* special case for empty array */
	if (type->regions[0].size == 0) {
		WARN_ON(type->cnt != 0 || type->total_size);
		type->regions[0].base = base;
		type->regions[0].size = size;
		type->regions[0].flags = flags;
		memblock_set_region_node(&type->regions[0], nid);
		type->total_size = size;
		type->cnt = 1;
		return 0;
	}

	/*
	 * The worst case is when new range overlaps all existing regions,
	 * then we'll need type->cnt + 1 empty regions in @type. So if
	 * type->cnt * 2 + 1 is less than or equal to type->max, we know
	 * that there is enough empty regions in @type, and we can insert
	 * regions directly.
	 */
	if (type->cnt * 2 + 1 <= type->max)
		insert = true;

repeat:
	/*
	 * The following is executed twice.  Once with %false @insert and
	 * then with %true.  The first counts the number of regions needed
	 * to accommodate the new area.  The second actually inserts them.
	 */
	base = obase;
	nr_new = 0;

	for_each_memblock_type(idx, type, rgn) {
		phys_addr_t rbase = rgn->base;
		phys_addr_t rend = rbase + rgn->size;

		if (rbase >= end)
			break;
		if (rend <= base)
			continue;
		/*
		 * @rgn overlaps.  If it separates the lower part of new
		 * area, insert that portion.
		 */
		if (rbase > base) {
#ifdef CONFIG_NUMA
			WARN_ON(nid != memblock_get_region_node(rgn));
#endif
			WARN_ON(flags != MEMBLOCK_NONE && flags != rgn->flags);
			nr_new++;
			if (insert) {
				if (start_rgn == -1)
					start_rgn = idx;
				end_rgn = idx + 1;
				memblock_insert_region(type, idx++, base,
						       rbase - base, nid,
						       flags);
			}
		}
		/* area below @rend is dealt with, forget about it */
		base = min(rend, end);
	}

	/* insert the remaining portion */
	if (base < end) {
		nr_new++;
		if (insert) {
			if (start_rgn == -1)
				start_rgn = idx;
			end_rgn = idx + 1;
			memblock_insert_region(type, idx, base, end - base,
					       nid, flags);
		}
	}

	if (!nr_new)
		return 0;

	/*
	 * If this was the first round, resize array and repeat for actual
	 * insertions; otherwise, merge and return.
	 */
	if (!insert) {
		while (type->cnt + nr_new > type->max)
			if (memblock_double_array(type, obase, size) < 0)
				return -ENOMEM;
		insert = true;
		goto repeat;
	} else {
		memblock_merge_regions(type, start_rgn, end_rgn);
		return 0;
	}
}
/* …（同文件无关代码省略）… */
static int __init_memblock memblock_isolate_range(struct memblock_type *type,
					phys_addr_t base, phys_addr_t size,
					int *start_rgn, int *end_rgn)
{
	phys_addr_t end = base + memblock_cap_size(base, &size);
	int idx;
	struct memblock_region *rgn;

	*start_rgn = *end_rgn = 0;

	if (!size)
		return 0;

	/* we'll create at most two more regions */
	while (type->cnt + 2 > type->max)
		if (memblock_double_array(type, base, size) < 0)
			return -ENOMEM;

	for_each_memblock_type(idx, type, rgn) {
		phys_addr_t rbase = rgn->base;
		phys_addr_t rend = rbase + rgn->size;

		if (rbase >= end)
			break;
		if (rend <= base)
			continue;

		if (rbase < base) {
			/*
			 * @rgn intersects from below.  Split and continue
			 * to process the next region - the new top half.
			 */
			rgn->base = base;
			rgn->size -= base - rbase;
			type->total_size -= base - rbase;
			memblock_insert_region(type, idx, rbase, base - rbase,
					       memblock_get_region_node(rgn),
					       rgn->flags);
		} else if (rend > end) {
			/*
			 * @rgn intersects from above.  Split and redo the
			 * current region - the new bottom half.
			 */
			rgn->base = end;
			rgn->size -= end - rbase;
			type->total_size -= end - rbase;
			memblock_insert_region(type, idx--, rbase, end - rbase,
					       memblock_get_region_node(rgn),
					       rgn->flags);
		} else {
			/* @rgn is fully contained, record it */
			if (!*end_rgn)
				*start_rgn = idx;
			*end_rgn = idx + 1;
		}
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int __init_memblock memblock_remove_range(struct memblock_type *type,
					  phys_addr_t base, phys_addr_t size)
{
	int start_rgn, end_rgn;
	int i, ret;

	ret = memblock_isolate_range(type, base, size, &start_rgn, &end_rgn);
	if (ret)
		return ret;

	for (i = end_rgn - 1; i >= start_rgn; i--)
		memblock_remove_region(type, i);
	return 0;
}
/* …（同文件无关代码省略）… */
static unsigned long __free_reserved_area(phys_addr_t start, phys_addr_t end,
					  int poison)
{
	unsigned long pages = 0, pfn;

	if (deferred_pages_enabled()) {
		WARN(1, "Cannot free reserved memory because of deferred initialization of the memory map");
		return 0;
	}

	for_each_valid_pfn(pfn, PFN_UP(start), PFN_DOWN(end)) {
		struct page *page = pfn_to_page(pfn);
		void *direct_map_addr;

		/*
		 * 'direct_map_addr' might be different from the kernel virtual
		 * address because some architectures use aliases.
		 * Going via physical address, pfn_to_page() and page_address()
		 * ensures that we get a _writeable_ alias for the memset().
		 */
		direct_map_addr = page_address(page);
		/*
		 * Perform a kasan-unchecked memset() since this memory
		 * has not been initialized.
		 */
		direct_map_addr = kasan_reset_tag(direct_map_addr);
		if ((unsigned int)poison <= 0xFF)
			memset(direct_map_addr, poison, PAGE_SIZE);

		free_reserved_page(page);
		pages++;
	}
	return pages;
}
/* …（同文件无关代码省略）… */
void __init_memblock memblock_free(void *ptr, size_t size)
{
	if (ptr)
		memblock_phys_free(__pa(ptr), size);
}
/* …（同文件无关代码省略）… */
int __init_memblock memblock_phys_free(phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;
	int ret = 0;

	memblock_dbg("%s: [%pa-%pa] %pS\n", __func__,
		     &base, &end, (void *)_RET_IP_);

	kmemleak_free_part_phys(base, size);

	if (!slab_is_available() || IS_ENABLED(CONFIG_ARCH_KEEP_MEMBLOCK))
		ret = memblock_remove_range(&memblock.reserved, base, size);

	if (slab_is_available())
		__free_reserved_area(base, base + size, -1);

	return ret;
}
/* …（同文件无关代码省略）… */
int __init_memblock __memblock_reserve(phys_addr_t base, phys_addr_t size,
				       int nid, enum memblock_flags flags)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("%s: [%pa-%pa] nid=%d flags=%x %pS\n", __func__,
		     &base, &end, nid, flags, (void *)_RET_IP_);

	return memblock_add_range(&memblock.reserved, base, size, nid, flags);
}
/* …（同文件无关代码省略）… */
static void memblock_prep_allocation(phys_addr_t start, phys_addr_t size,
				     bool kmemleak_trace)
{
	/*
	 * Skip kmemleak for those places like kasan_init() and
	 * early_pgtable_alloc() due to high volume.
	 */
	if (kmemleak_trace)
		/*
		 * Memblock allocated blocks are never reported as
		 * leaks. This is because many of these blocks are
		 * only referred via the physical address which is
		 * not looked up by kmemleak.
		 */
		kmemleak_alloc_phys(start, size, 0);

	/*
	 * Some Virtual Machine platforms, such as Intel TDX or AMD SEV-SNP,
	 * require memory to be accepted before it can be used by the
	 * guest.
	 *
	 * Accept the memory of the allocated buffer.
	 */
	accept_memory(start, size);
}
/* …（同文件无关代码省略）… */
phys_addr_t __init memblock_alloc_range_nid(phys_addr_t size,
					phys_addr_t align, phys_addr_t start,
					phys_addr_t end, int nid,
					bool exact_nid)
{
	enum memblock_flags flags = choose_memblock_flags();
	phys_addr_t found;

	/*
	 * Detect any accidental use of these APIs after slab is ready, as at
	 * this moment memblock may be deinitialized already and its
	 * internal data may be destroyed (after execution of memblock_free_all)
	 */
	if (WARN_ON_ONCE(slab_is_available())) {
		void *vaddr = kzalloc_node(size, GFP_NOWAIT, nid);

		return vaddr ? virt_to_phys(vaddr) : 0;
	}

	if (!align) {
		/* Can't use WARNs this early in boot on powerpc */
		dump_stack();
		align = SMP_CACHE_BYTES;
	}

again:
	found = memblock_find_in_range_node(size, align, start, end, nid,
					    flags);
	if (found && !__memblock_reserve(found, size, nid, MEMBLOCK_RSRV_KERN))
		goto done;

	if (numa_valid_node(nid) && !exact_nid) {
		found = memblock_find_in_range_node(size, align, start,
						    end, NUMA_NO_NODE,
						    flags);
		if (found && !memblock_reserve_kern(found, size))
			goto done;
	}

	if (flags & MEMBLOCK_MIRROR) {
		flags &= ~MEMBLOCK_MIRROR;
		pr_warn_ratelimited("Could not allocate %pap bytes of mirrored memory\n",
			&size);
		goto again;
	}

	return 0;

done:
	memblock_prep_allocation(found, size, end != MEMBLOCK_ALLOC_NOLEAKTRACE);
	return found;
}
/* …（同文件无关代码省略）… */
 * @min_addr: the lower bound of the memory region to allocate (phys address)
 * @max_addr: the upper bound of the memory region to allocate (phys address)
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 * @exact_nid: control the allocation fall back to other nodes
 *
 * Allocates memory block using memblock_alloc_range_nid() and
 * converts the returned physical address to virtual.
/* …（同文件无关代码省略）… */
static void * __init memblock_alloc_internal(
				phys_addr_t size, phys_addr_t align,
				phys_addr_t min_addr, phys_addr_t max_addr,
				int nid, bool exact_nid)
{
	phys_addr_t alloc;

/* …（同文件无关代码省略）… */
		max_addr = memblock.current_limit;

	alloc = memblock_alloc_range_nid(size, align, min_addr, max_addr, nid,
					exact_nid);

	/* retry allocation without lower limit */
	if (!alloc && min_addr)
		alloc = memblock_alloc_range_nid(size, align, 0, max_addr, nid,
						exact_nid);

	if (!alloc)
		return NULL;

	return phys_to_virt(alloc);
}

/**
 * memblock_alloc_exact_nid_raw - allocate boot memory block on the exact node
 * without zeroing memory
 * @size: size of memory block to be allocated in bytes
 * @align: alignment of the region and block's size
 * @min_addr: the lower bound of the memory region from where the allocation
 *	  is preferred (phys address)
 * @max_addr: the upper bound of the memory region from where the allocation
 *	      is preferred (phys address), or %MEMBLOCK_ALLOC_ACCESSIBLE to
 *	      allocate only from memory limited by memblock.current_limit value
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 *
 * Public function, provides additional debug information (including caller
 * info), if enabled. Does not zero allocated memory.
 *
 * Return:
 * Virtual address of allocated memory block on success, NULL on failure.
 */
void * __init memblock_alloc_exact_nid_raw(
			phys_addr_t size, phys_addr_t align,
			phys_addr_t min_addr, phys_addr_t max_addr,
			int nid)
{
	memblock_dbg("%s: %llu bytes align=0x%llx nid=%d from=%pa max_addr=%pa %pS\n",
		     __func__, (u64)size, (u64)align, nid, &min_addr,
		     &max_addr, (void *)_RET_IP_);

	return memblock_alloc_internal(size, align, min_addr, max_addr, nid,
				       true);
}

/**
 * memblock_alloc_try_nid_raw - allocate boot memory block without zeroing
 * memory and without panicking
/* …（同文件无关代码省略）… */
		     __func__, (u64)size, (u64)align, nid, &min_addr,
		     &max_addr, (void *)_RET_IP_);

	return memblock_alloc_internal(size, align, min_addr, max_addr, nid,
				       false);
}

/**
/* …（同文件无关代码省略）… */
	memblock_dbg("%s: %llu bytes align=0x%llx nid=%d from=%pa max_addr=%pa %pS\n",
		     __func__, (u64)size, (u64)align, nid, &min_addr,
		     &max_addr, (void *)_RET_IP_);
	ptr = memblock_alloc_internal(size, align,
					   min_addr, max_addr, nid, false);
	if (ptr)
		memset(ptr, 0, size);

/* …（同文件无关代码省略）… */

static int __init_memblock memblock_search(struct memblock_type *type, phys_addr_t addr)
{
	unsigned int left = 0, right = type->cnt;

	do {
		unsigned int mid = (right + left) / 2;

		if (addr < type->regions[mid].base)
			right = mid;
		else if (addr >= (type->regions[mid].base +
				  type->regions[mid].size))
			left = mid + 1;
		else
			return mid;
	} while (left < right);
	return -1;
}

/* …（同文件无关代码省略）… */
	phys_addr_t base, end, size;
	enum memblock_flags flags;
	int idx;
	struct memblock_region *rgn;

	pr_info(" %s.cnt  = 0x%lx\n", type->name, type->cnt);

	for_each_memblock_type(idx, type, rgn) {
		char nid_buf[32] = "";

		base = rgn->base;
/* …（同文件无关代码省略）… */
	return count;
}

static int reset_managed_pages_done __initdata;

static void __init reset_node_managed_pages(pg_data_t *pgdat)
{
	struct zone *z;
/* …（同文件无关代码省略）… */
		atomic_long_set(&z->managed_pages, 0);
}

void __init reset_all_zones_managed_pages(void)
{
	struct pglist_data *pgdat;

	if (reset_managed_pages_done)
		return;

	for_each_online_pgdat(pgdat)
		reset_node_managed_pages(pgdat);

	reset_managed_pages_done = 1;
}

/**
/* …（同文件无关代码省略）… */
#define RESERVE_MEM_NAME_SIZE		16
struct reserve_mem_table {
	char			name[RESERVE_MEM_NAME_SIZE];
	phys_addr_t		start;
	phys_addr_t		size;
};
/* …（同文件无关代码省略）… */
static int __init reserved_mem_preserve(void)
{
	unsigned int nr_preserved = 0;
	int err;

	for (unsigned int i = 0; i < reserved_mem_count; i++, nr_preserved++) {
		struct reserve_mem_table *map = &reserved_mem_table[i];
		struct page *page = phys_to_page(map->start);
		unsigned int nr_pages = map->size >> PAGE_SHIFT;

		err = kho_preserve_pages(page, nr_pages);
		if (err)
			goto err_unpreserve;
	}

	return 0;

err_unpreserve:
	for (unsigned int i = 0; i < nr_preserved; i++) {
		struct reserve_mem_table *map = &reserved_mem_table[i];
		struct page *page = phys_to_page(map->start);
		unsigned int nr_pages = map->size >> PAGE_SHIFT;

		kho_unpreserve_pages(page, nr_pages);
	}

	return err;
}
/* …（同文件无关代码省略）… */
static int __init prepare_kho_fdt(void)
{
	struct page *fdt_page;
	void *fdt;
	int err;

	fdt_page = alloc_page(GFP_KERNEL);
	if (!fdt_page) {
		err = -ENOMEM;
		goto err_report;
	}

	fdt = page_to_virt(fdt_page);
	err = kho_preserve_pages(fdt_page, 1);
	if (err)
		goto err_free_fdt;

	err |= fdt_create(fdt, PAGE_SIZE);
	err |= fdt_finish_reservemap(fdt);
	err |= fdt_begin_node(fdt, "");
	err |= fdt_property_string(fdt, "compatible", MEMBLOCK_KHO_NODE_COMPATIBLE);

	for (unsigned int i = 0; !err && i < reserved_mem_count; i++) {
		struct reserve_mem_table *map = &reserved_mem_table[i];

		err |= fdt_begin_node(fdt, map->name);
		err |= fdt_property_string(fdt, "compatible", RESERVE_MEM_KHO_NODE_COMPATIBLE);
		err |= fdt_property(fdt, "start", &map->start, sizeof(map->start));
		err |= fdt_property(fdt, "size", &map->size, sizeof(map->size));
		err |= fdt_end_node(fdt);
	}
	err |= fdt_end_node(fdt);
	err |= fdt_finish(fdt);

	if (err)
		goto err_unpreserve_fdt;

	err = kho_add_subtree(MEMBLOCK_KHO_FDT, fdt, fdt_totalsize(fdt));
	if (err)
		goto err_unpreserve_fdt;

	err = reserved_mem_preserve();
	if (err)
		goto err_remove_subtree;

	return 0;

err_remove_subtree:
	kho_remove_subtree(fdt);
err_unpreserve_fdt:
	kho_unpreserve_pages(fdt_page, 1);
err_free_fdt:
	put_page(fdt_page);
err_report:
	pr_err("failed to prepare memblock FDT for KHO: %d\n", err);

	return err;
}

static int __init reserve_mem_init(void)
{
	int err;

	if (!kho_is_enabled() || !reserved_mem_count)
		return 0;

	err = prepare_kho_fdt();
	if (err)
		return err;
	return err;
}
late_initcall(reserve_mem_init);

/* …（同文件无关代码省略）… */
	if (*p != ':')
		goto err_param;

	/*
	 * memblock_phys_alloc() doesn't like a zero size align,
	 * but it is OK for this command to have it.
	 */
	if (align < SMP_CACHE_BYTES)
		align = SMP_CACHE_BYTES;

	name = p + 1;
	len = strlen(name);
