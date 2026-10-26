// AUTO-DRAFT from torvalds/linux PR #126ad898a6e97fcb4c3fbe6d979829f72b8c348c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR

#include <linux/cleanup.h>
#include <linux/cma.h>
#include <linux/kmemleak.h>
#include <linux/count_zeros.h>
#include <linux/kasan.h>
/* …（同文件无关代码省略）… */
/*
 * This is the minimal alignment required by deferred struct page init.
 * deferred_init_memmap_chunk frees memory to the buddy allocator, which looks
 * at the neighboring pages (up to MAX_PAGE_ORDER) to merge them.
 * If KHO scratch is not aligned to that value, buddy can access uninitialized
 * struct pages, which can cause a crash.
 */
#define SCRATCH_ALIGNMENT_BYTES (PAGE_SIZE * MAX_ORDER_NR_PAGES)
static_assert(SCRATCH_ALIGNMENT_BYTES >= CMA_MIN_ALIGNMENT_BYTES);

/* The magic token for preserved pages */
#define KHO_PAGE_MAGIC 0x4b484f50U /* ASCII for 'KHOP' */
/* …（同文件无关代码省略）… */
union kho_page_info {
	unsigned long page_private;
	struct {
		unsigned int order;
		unsigned int magic;
	};
};
/* …（同文件无关代码省略）… */
struct kho_out {
	void *fdt;
	struct mutex lock; /* protects KHO FDT */

	struct kho_radix_tree radix_tree;
	struct kho_debugfs dbg;
};
/* …（同文件无关代码省略）… */

struct kho_in {
	phys_addr_t fdt_phys;
	phys_addr_t scratch_phys;
	char previous_release[__NEW_UTS_LEN + 1];
	u32 kexec_count;
	struct kho_debugfs dbg;
/* …（同文件无关代码省略）… */
static unsigned long kho_encode_radix_key(phys_addr_t phys, unsigned int order)
{
	/* The physical address is encoded by shifting the PFN by its order. */
	unsigned long shift = PAGE_SHIFT + order;
	/* Order bit goes right before the shifted PFN. */
	unsigned long h = 1UL << (64 - shift);
	/* Shifted PFN. */
	unsigned long l = phys >> shift;

	return h | l;
}
/* …（同文件无关代码省略）… */
static phys_addr_t kho_decode_radix_key(unsigned long key, unsigned int *order)
{
	/* fls64() indexes starting from 1. */
	unsigned int order_bit = fls64(key) - 1;
	phys_addr_t phys;

	/* order bit goes right before the shifted PFN. */
	*order = 64 - (PAGE_SHIFT + order_bit);
	/* The order bit is discarded by the shift */
	phys = key << (PAGE_SHIFT + *order);

	return phys;
}
/* …（同文件无关代码省略）… */
static unsigned long kho_radix_get_bitmap_index(unsigned long key)
{
	return key % (1 << KHO_BITMAP_SIZE_LOG2);
}
/* …（同文件无关代码省略）… */
static unsigned long kho_radix_get_table_index(unsigned long key,
					       unsigned int level)
{
	int s;

	s = ((level - 1) * KHO_TABLE_SIZE_LOG2) + KHO_BITMAP_SIZE_LOG2;
	return (key >> s) % (1 << KHO_TABLE_SIZE_LOG2);
}
/* …（同文件无关代码省略）… */
static void __ref *kho_radix_alloc_node(void)
{
	struct kho_radix_node *node;

	if (slab_is_available())
		node = (struct kho_radix_node *)get_zeroed_page(GFP_KERNEL);
	else
		node = memblock_alloc(PAGE_SIZE, PAGE_SIZE);

	return node;
}
/* …（同文件无关代码省略）… */
static void __ref kho_radix_free_node(struct kho_radix_node *node)
{
	if (slab_is_available())
		free_page((unsigned long)node);
	else
		memblock_free(node, PAGE_SIZE);
}
/* …（同文件无关代码省略）… */
int kho_radix_add_key(struct kho_radix_tree *tree, unsigned long key)
{
	/* Newly allocated nodes for error cleanup */
	struct kho_radix_node *intermediate_nodes[KHO_TREE_MAX_DEPTH] = { 0 };
	struct kho_radix_node *anchor_node = NULL;
	struct kho_radix_node *node = tree->root;
	struct kho_radix_node *new_node;
	unsigned int i, idx, anchor_idx;
	struct kho_radix_leaf *leaf;
	int err = 0;

	if (WARN_ON_ONCE(!tree->root))
		return -EINVAL;

	if (unlikely(fls64(key) > KHO_RADIX_KEY_WIDTH))
		return -ERANGE;

	might_sleep();

	guard(mutex)(&tree->lock);

	/* Go from high levels to low levels */
	for (i = KHO_TREE_MAX_DEPTH - 1; i > 0; i--) {
		idx = kho_radix_get_table_index(key, i);

		if (node->table[idx]) {
			node = phys_to_virt(node->table[idx]);
			continue;
		}

		/* Next node is empty, create a new node for it */
		new_node = kho_radix_alloc_node();
		if (!new_node) {
			err = -ENOMEM;
			goto err_free_nodes;
		}

		node->table[idx] = virt_to_phys(new_node);

		/*
		 * Capture the node where the new branch starts for cleanup
		 * if allocation fails.
		 */
		if (!anchor_node) {
			anchor_node = node;
			anchor_idx = idx;
		}
		intermediate_nodes[i] = new_node;

		node = new_node;
	}

	/* Handle the leaf level bitmap (level 0) */
	idx = kho_radix_get_bitmap_index(key);
	leaf = (struct kho_radix_leaf *)node;
	__set_bit(idx, leaf->bitmap);

	return 0;

err_free_nodes:
	for (i = KHO_TREE_MAX_DEPTH - 1; i > 0; i--) {
		if (intermediate_nodes[i])
			kho_radix_free_node(intermediate_nodes[i]);
	}
	if (anchor_node)
		anchor_node->table[anchor_idx] = 0;

	return err;
}
/* …（同文件无关代码省略）… */
void kho_radix_del_key(struct kho_radix_tree *tree, unsigned long key)
{
	struct kho_radix_node *node = tree->root;
	struct kho_radix_leaf *leaf;
	unsigned int i, idx;

	if (WARN_ON_ONCE(!tree->root))
		return;

	/* Keys wider than KHO_RADIX_KEY_WIDTH are not allowed to be added. */
	if (unlikely(fls64(key) > KHO_RADIX_KEY_WIDTH))
		return;

	might_sleep();

	guard(mutex)(&tree->lock);

	/* Go from high levels to low levels */
	for (i = KHO_TREE_MAX_DEPTH - 1; i > 0; i--) {
		idx = kho_radix_get_table_index(key, i);

		/*
		 * Attempting to delete a page that has not been preserved,
		 * return with a warning.
		 */
		if (WARN_ON(!node->table[idx]))
			return;

		node = phys_to_virt(node->table[idx]);
	}

	/* Handle the leaf level bitmap (level 0) */
	leaf = (struct kho_radix_leaf *)node;
	idx = kho_radix_get_bitmap_index(key);
	__clear_bit(idx, leaf->bitmap);
}
/* …（同文件无关代码省略）… */
static void __kho_radix_destroy_tree(struct kho_radix_node *root,
				     unsigned int level)
{
	unsigned long i;

	if (level == 0) {
		kho_radix_free_node(root);
		return;
	}

	for (i = 0; i < PAGE_SIZE / sizeof(phys_addr_t); i++) {
		if (root->table[i])
			__kho_radix_destroy_tree(phys_to_virt(root->table[i]),
						 level - 1);
	}

	kho_radix_free_node(root);
}
/* …（同文件无关代码省略）… */
int kho_radix_init_tree(struct kho_radix_tree *tree, struct kho_radix_node *root)
{
	if (!root)
		root = kho_radix_alloc_node();
	if (!root)
		return -ENOMEM;

	tree->root = root;
	mutex_init(&tree->lock);
	return 0;
}
/* …（同文件无关代码省略）… */
void kho_radix_destroy_tree(struct kho_radix_tree *tree)
{
	if (!tree->root)
		return;

	__kho_radix_destroy_tree(tree->root, KHO_TREE_MAX_DEPTH - 1);
	tree->root = NULL;
}
/* …（同文件无关代码省略）… */
static int kho_radix_walk_leaf(struct kho_radix_leaf *leaf, unsigned long key,
			       const struct kho_radix_walk_cb *cb, void *data)
{
	unsigned long *bitmap = (unsigned long *)leaf;
	unsigned int i;
	int err;

	if (cb->node) {
		err = cb->node(virt_to_phys(leaf), data);
		if (err)
			return err;
	}

	if (!cb->leaf)
		return 0;

	for_each_set_bit(i, bitmap, PAGE_SIZE * BITS_PER_BYTE) {
		err = cb->leaf(key | i, data);
		if (err)
			return err;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int __kho_radix_walk_tree(struct kho_radix_node *root,
				 unsigned int level, unsigned long start,
				 const struct kho_radix_walk_cb *cb, void *data)
{
	struct kho_radix_node *node;
	struct kho_radix_leaf *leaf;
	unsigned long key, i;
	unsigned int shift;
	int err;

	if (cb->node) {
		err = cb->node(virt_to_phys(root), data);
		if (err)
			return err;
	}

	for (i = 0; i < PAGE_SIZE / sizeof(phys_addr_t); i++) {
		if (!root->table[i])
			continue;

		shift = ((level - 1) * KHO_TABLE_SIZE_LOG2) +
			KHO_BITMAP_SIZE_LOG2;
		key = start | (i << shift);

		node = phys_to_virt(root->table[i]);

		if (level == 1) {
			/*
			 * we are at level 1,
			 * node is pointing to the level 0 bitmap.
			 */
			leaf = (struct kho_radix_leaf *)node;
			err = kho_radix_walk_leaf(leaf, key, cb, data);
		} else {
			err  = __kho_radix_walk_tree(node, level - 1,
						     key, cb, data);
		}

		if (err)
			return err;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
int kho_radix_walk_tree(struct kho_radix_tree *tree,
			const struct kho_radix_walk_cb *cb, void *data)
{
	if (WARN_ON_ONCE(!tree->root))
		return -EINVAL;

	guard(mutex)(&tree->lock);

	return __kho_radix_walk_tree(tree->root, KHO_TREE_MAX_DEPTH - 1, 0, cb,
				     data);
}
/* …（同文件无关代码省略）… */
static struct page *__init kho_get_preserved_page(phys_addr_t phys,
						  unsigned int order)
{
	unsigned long pfn = PHYS_PFN(phys);
	int nid;

	if (!IS_ENABLED(CONFIG_DEFERRED_STRUCT_PAGE_INIT))
		return pfn_to_page(pfn);

	nid = early_pfn_to_nid(pfn);
	for (unsigned long i = 0; i < (1UL << order); i++)
		init_deferred_page(pfn + i, nid);

	return pfn_to_page(pfn);
}
/* …（同文件无关代码省略）… */
static int __init kho_preserved_memory_reserve(unsigned long key, void *data)
{
	union kho_page_info info;
	struct page *page;
	unsigned int order;
	phys_addr_t phys;
	u64 sz;

	phys = kho_decode_radix_key(key, &order);

	sz = 1UL << (order + PAGE_SHIFT);
	page = kho_get_preserved_page(phys, order);

	/* Reserve the memory preserved in KHO in memblock */
	memblock_reserve(phys, sz);
	memblock_reserved_mark_noinit(phys, sz);
	info.magic = KHO_PAGE_MAGIC;
	info.order = order;
	page->private = info.page_private;

	return 0;
}
/* …（同文件无关代码省略）… */
static phys_addr_t __init kho_get_mem_map_phys(const void *fdt)
{
	const void *mem_ptr;
	int len;

	mem_ptr = fdt_getprop(fdt, 0, KHO_FDT_MEMORY_MAP_PROP_NAME, &len);
	if (!mem_ptr || len != sizeof(u64)) {
		pr_err("failed to get preserved memory map\n");
		return 0;
	}

	return get_unaligned((const u64 *)mem_ptr);
}
/* …（同文件无关代码省略）… */
static void __init *kho_get_mem_map(const void *fdt)
{
	phys_addr_t phys = kho_get_mem_map_phys(fdt);

	return phys ? phys_to_virt(phys) : NULL;
}

/*
 * With KHO enabled, memory can become fragmented because KHO regions may
 * be anywhere in physical address space. The scratch regions give us a
 * safe zones that we will never see KHO allocations from. This is where we
 * can later safely load our new kexec images into and then use the scratch
 * area for early allocations that happen before page allocator is
 * initialized.
 */
struct kho_scratch *kho_scratch;
unsigned int kho_scratch_cnt;

/*
 * The scratch areas are scaled by default as percent of memory allocated from
 * memblock. A user can override the scale with command line parameter:
 *
 * kho_scratch=N%
 *
 * It is also possible to explicitly define size for a lowmem, a global and
 * per-node scratch areas:
 *
 * kho_scratch=l[KMG],n[KMG],m[KMG]
 *
 * The explicit size definition takes precedence over scale definition.
 */
static unsigned int scratch_scale __initdata = 200;
static phys_addr_t scratch_size_global __initdata;
static phys_addr_t scratch_size_pernode __initdata;
static phys_addr_t scratch_size_lowmem __initdata;

static int __init kho_parse_scratch_size(char *p)
{
	size_t len;
	unsigned long sizes[3];
/* …（同文件无关代码省略）… */
			return -EINVAL;

		memcpy(s_scale, p, len - 1);
		ret = kstrtouint(s_scale, 10, &scratch_scale);
		if (!ret)
			pr_notice("scratch scale is %d%%\n", scratch_scale);
		return ret;
	}

/* …（同文件无关代码省略）… */
	if (*p)
		return -EINVAL;

	scratch_size_lowmem = sizes[0];
	scratch_size_global = sizes[1];
	scratch_size_pernode = sizes[2];
	scratch_scale = 0;

	pr_notice("scratch areas: lowmem: %lluMiB global: %lluMiB pernode: %lldMiB\n",
		  (u64)(scratch_size_lowmem >> 20),
		  (u64)(scratch_size_global >> 20),
		  (u64)(scratch_size_pernode >> 20));

	return 0;
}
early_param("kho_scratch", kho_parse_scratch_size);

static void __init scratch_size_update(void)
{
	/*
	 * If fixed sizes are not provided via command line, calculate them now.
	 * Remove HugeTLB allocations from it because they never get allocated
	 * from scratch.
	 */
	if (scratch_scale) {
		phys_addr_t size;

		size = memblock_reserved_kern_size(ARCH_LOW_ADDRESS_LIMIT,
						   NUMA_NO_NODE);
		size -= memblock_reserved_hugetlb_size(ARCH_LOW_ADDRESS_LIMIT,
						       NUMA_NO_NODE);
		size = size * scratch_scale / 100;
		scratch_size_lowmem = size;

		size = memblock_reserved_kern_size(MEMBLOCK_ALLOC_ANYWHERE,
						   NUMA_NO_NODE);
		size -= memblock_reserved_hugetlb_size(MEMBLOCK_ALLOC_ANYWHERE,
						       NUMA_NO_NODE);
		size = size * scratch_scale / 100 - scratch_size_lowmem;
		scratch_size_global = size;
	}

	/*
	 * Scratch areas are released as MIGRATE_CMA. Round them up to the right
	 * size.
	 */
	scratch_size_lowmem = round_up(scratch_size_lowmem, SCRATCH_ALIGNMENT_BYTES);
	scratch_size_global = round_up(scratch_size_global, SCRATCH_ALIGNMENT_BYTES);
}

static phys_addr_t __init scratch_size_node(int nid)
{
	phys_addr_t size;

	if (scratch_scale) {
		size = memblock_reserved_kern_size(MEMBLOCK_ALLOC_ANYWHERE,
						   nid);
		/* Do not count HugeTLB pages. */
		size -= memblock_reserved_hugetlb_size(MEMBLOCK_ALLOC_ANYWHERE,
						       nid);
		size = size * scratch_scale / 100;
	} else {
		size = scratch_size_pernode;
	}

	return round_up(size, SCRATCH_ALIGNMENT_BYTES);
}

bool kho_scratch_overlap(phys_addr_t phys, size_t size)
{
	phys_addr_t scratch_start, scratch_end;
	unsigned int i;

	for (i = 0; i < kho_scratch_cnt; i++) {
		scratch_start = kho_scratch[i].addr;
		scratch_end = kho_scratch[i].addr + kho_scratch[i].size;

		if (phys < scratch_end && (phys + size) > scratch_start)
			return true;
	}

	return false;
}

/**
 * kho_reserve_scratch - Reserve a contiguous chunk of memory for kexec
 *
 * With KHO we can preserve arbitrary pages in the system. To ensure we still
 * have a large contiguous region of memory when we search the physical address
 * space for target memory, let's make sure we always have a large CMA region
 * active. This CMA region will only be used for movable pages which are not a
 * problem for us during KHO because we can just move them somewhere else.
 */
static void __init kho_reserve_scratch(void)
{
	phys_addr_t addr, size;
	int nid, i = 0;

	if (!kho_enable)
		return;

	scratch_size_update();

	/* FIXME: deal with node hot-plug/remove */
	kho_scratch_cnt = nodes_weight(node_states[N_MEMORY]) + 2;
	size = kho_scratch_cnt * sizeof(*kho_scratch);
	kho_scratch = memblock_alloc(size, PAGE_SIZE);
	if (!kho_scratch) {
		pr_err("Failed to reserve scratch array\n");
		goto err_disable_kho;
	}

	/*
	 * reserve scratch area in low memory for lowmem allocations in the
	 * next kernel
	 */
	size = scratch_size_lowmem;
	addr = memblock_phys_alloc_range(size, SCRATCH_ALIGNMENT_BYTES, 0,
					 ARCH_LOW_ADDRESS_LIMIT);
	if (!addr) {
		pr_err("Failed to reserve lowmem scratch buffer\n");
		goto err_free_scratch_desc;
	}

	kho_scratch[i].addr = addr;
	kho_scratch[i].size = size;
	i++;

	/* reserve large contiguous area for allocations without nid */
	size = scratch_size_global;
	addr = memblock_phys_alloc(size, SCRATCH_ALIGNMENT_BYTES);
	if (!addr) {
		pr_err("Failed to reserve global scratch buffer\n");
		goto err_free_scratch_areas;
	}

	kho_scratch[i].addr = addr;
	kho_scratch[i].size = size;
	i++;

	/*
	 * Loop over nodes that have both memory and are online. Skip
	 * memoryless nodes, as we can not allocate scratch areas there.
	 */
	for_each_node_state(nid, N_MEMORY) {
		size = scratch_size_node(nid);
		addr = memblock_alloc_range_nid(size, SCRATCH_ALIGNMENT_BYTES,
						0, MEMBLOCK_ALLOC_ACCESSIBLE,
						nid, true);
		if (!addr) {
			pr_err("Failed to reserve nid %d scratch buffer\n", nid);
			goto err_free_scratch_areas;
		}

		kho_scratch[i].addr = addr;
		kho_scratch[i].size = size;
		i++;
	}

	return;

err_free_scratch_areas:
	for (i--; i >= 0; i--)
		memblock_phys_free(kho_scratch[i].addr, kho_scratch[i].size);
err_free_scratch_desc:
	memblock_free(kho_scratch, kho_scratch_cnt * sizeof(*kho_scratch));
err_disable_kho:
	pr_warn("Failed to reserve scratch area, disabling kexec handover\n");
	kho_enable = false;
}

/* …（同文件无关代码省略）… */
 * on smaller systems. The algorithm itself doesn't depend on the actual value,
 * so it can be changed to a different heuristic later if needed.
 */
#define KHO_SCRATCH_EXT_BLKSIZE		SZ_1G
#define KHO_SCRATCH_EXT_BLKSHIFT	const_ilog2(KHO_SCRATCH_EXT_BLKSIZE)

/* Called for the KHO preserved memory radix tree. */
static int __init kho_ext_walk_leaf(unsigned long key, void *data)
{
	struct kho_radix_tree *busy_blocks = data;
	phys_addr_t start, end;
	unsigned int order;
	int err;

	/*
	 * The key is from the KHO preserved memory radix tree. It is decoded to
	 * a physical address of a preservation and its order.
	 */
	start = kho_decode_radix_key(key, &order);
	end = start + (1UL << (order + PAGE_SHIFT));

	while (start < end) {
		err = kho_radix_add_key(busy_blocks, start >> KHO_SCRATCH_EXT_BLKSHIFT);
		if (err)
			return err;

		start += (1UL << KHO_SCRATCH_EXT_BLKSHIFT);
	}

	return 0;
}

/* Called for the KHO preserved memory radix tree. */
static int __init kho_ext_walk_node(phys_addr_t phys, void *data)
{
	struct kho_radix_tree *busy_blocks = data;

	return kho_radix_add_key(busy_blocks, phys >> KHO_SCRATCH_EXT_BLKSHIFT);
}

/* Called for the busy block radix tree. */
static int __init kho_ext_mark_scratch(unsigned long key, void *data)
{
	phys_addr_t *prev_end = data;
	phys_addr_t start = key << KHO_SCRATCH_EXT_BLKSHIFT;
	int err;

	if (start > *prev_end) {
		err = memblock_mark_kho_scratch(*prev_end, start - *prev_end);
		if (err)
			return err;
	}

	*prev_end = start + (1UL << KHO_SCRATCH_EXT_BLKSHIFT);
	return 0;
}

/*
 * kho_extend_scratch - Extend the scratch regions
 *
 * The KHO preserved memory radix tree mixes both physical address and order
 * into a single key. This makes it hard to look for free ranges directly. This
 * function first walks the radix tree and digests it down into another radix
 * tree, whose keys identify blocks of size KHO_SCRATCH_EXT_BLKSIZE which
 * contain preserved memory.
 *
 * Then it walks the digested radix tree and marks everything that doesn't have
 * preserved memory as scratch.
 *
 * NOTE: This function allocates memory so it should be called when scratch has
 * available space.
 *
 * NOTE: The pages of the KHO preserved memory radix tree tables are not marked
/* …（同文件无关代码省略）… */
 * them to be "preserved memory" and marks their blocks as busy.
 *
 * NOTE: efi_init()::reserve_regions() removes all regions except
 * MEMBLOCK_KHO_SCRATCH. This function adds such regions but they are not KHO
 * scratch memory, so they should not be removed. This function should always be
 * called after reserve_regions().
 */
static void __init kho_extend_scratch(void)
{
	const struct kho_radix_walk_cb kho_cb = {
		.leaf = kho_ext_walk_leaf,
		.node = kho_ext_walk_node,
	};
	const struct kho_radix_walk_cb ext_cb = {
		.leaf = kho_ext_mark_scratch,
	};
	static struct lock_class_key busy_radix_class;
	struct kho_radix_tree busy_blocks;
/* …（同文件无关代码省略）… */
	if (err)
		goto out;

	/* Walk the busy blocks and mark everything between keys as scratch. */
	err = kho_radix_walk_tree(&busy_blocks, &ext_cb, &prev_end);
	if (err)
		goto out;

	/* Mark everything from last busy block to end of DRAM. */
	if (prev_end < memblock_end_of_DRAM())
		err = memblock_mark_kho_scratch(prev_end, memblock_end_of_DRAM() - prev_end);

	/* fallthrough */
out:
	kho_radix_destroy_tree(&busy_blocks);
print:
	if (err)
		pr_err("Failed to extend scratch: %pe\n", ERR_PTR(err));
}

/**
/* …（同文件无关代码省略）… */
int kho_preserve_folio(struct folio *folio)
{
	struct kho_radix_tree *tree = &kho_out.radix_tree;
	const unsigned long pfn = folio_pfn(folio);
	const unsigned int order = folio_order(folio);

	if (IS_ENABLED(CONFIG_KEXEC_HANDOVER_DEBUG) &&
	    WARN_ON(kho_scratch_overlap(pfn << PAGE_SHIFT, PAGE_SIZE << order)))
		return -EINVAL;

	return kho_radix_add_key(tree, kho_encode_radix_key(PFN_PHYS(pfn),
							    order));
}
/* …（同文件无关代码省略）… */
void kho_unpreserve_folio(struct folio *folio)
{
	struct kho_radix_tree *tree = &kho_out.radix_tree;
	const unsigned long pfn = folio_pfn(folio);
	const unsigned int order = folio_order(folio);

	kho_radix_del_key(tree, kho_encode_radix_key(PFN_PHYS(pfn), order));
}
/* …（同文件无关代码省略）… */
	int err = 0;

	if (IS_ENABLED(CONFIG_KEXEC_HANDOVER_DEBUG) &&
	    WARN_ON(kho_scratch_overlap(start_pfn << PAGE_SHIFT,
					nr_pages << PAGE_SHIFT))) {
		return -EINVAL;
	}
/* …（同文件无关代码省略）… */
void *kho_alloc_preserve(size_t size)
{
	struct folio *folio;
	int order, ret;

	if (!size)
		return ERR_PTR(-EINVAL);

	order = get_order(size);
	if (order > MAX_PAGE_ORDER)
		return ERR_PTR(-E2BIG);

	folio = folio_alloc(GFP_KERNEL | __GFP_ZERO, order);
	if (!folio)
		return ERR_PTR(-ENOMEM);

	ret = kho_preserve_folio(folio);
	if (ret) {
		folio_put(folio);
		return ERR_PTR(ret);
	}

	return folio_address(folio);
}
/* …（同文件无关代码省略）… */
void kho_unpreserve_free(void *mem)
{
	struct folio *folio;

	if (!mem)
		return;

	folio = virt_to_folio(mem);
	kho_unpreserve_folio(folio);
	folio_put(folio);
}
/* …（同文件无关代码省略）… */
static void __init kho_mem_retrieve(void)
{
	const struct kho_radix_walk_cb cb = {
		.leaf = kho_preserved_memory_reserve,
	};

	if (kho_radix_walk_tree(&kho_in.radix_tree, &cb, NULL))
		goto err;

	return;

err:
	/*
	 * Failed to initialize preserved memory. Clear FDT and radix so KHO
	 * users don't treat it as a KHO boot.
	 */
	kho_in.fdt_phys = 0;
	kho_in.radix_tree.root = NULL;
}
/* …（同文件无关代码省略）… */

	err = kho_radix_init_tree(tree, NULL);
	if (err)
		goto err_free_scratch;

	kho_out.fdt = kho_alloc_preserve(PAGE_SIZE);
	if (IS_ERR(kho_out.fdt)) {
/* …（同文件无关代码省略）… */
		return 0;
	}

	for (int i = 0; i < kho_scratch_cnt; i++) {
		unsigned long base_pfn = PHYS_PFN(kho_scratch[i].addr);
		unsigned long count = kho_scratch[i].size >> PAGE_SHIFT;
		unsigned long pfn;

		/*
		 * When debug_pagealloc is enabled, __free_pages() clears the
		 * corresponding PRESENT bit in the kernel page table.
		 * Subsequent kmemleak scans of these pages cause the
		 * non-PRESENT page faults.
		 * Mark scratch areas with kmemleak_ignore_phys() to exclude
		 * them from kmemleak scanning.
		 */
		kmemleak_ignore_phys(kho_scratch[i].addr);
		for (pfn = base_pfn; pfn < base_pfn + count;
		     pfn += pageblock_nr_pages)
			init_cma_reserved_pageblock(pfn_to_page(pfn));
/* …（同文件无关代码省略）… */
	kho_unpreserve_free(kho_out.fdt);
err_free_kho_radix_tree:
	kho_radix_destroy_tree(tree);
err_free_scratch:
	kho_out.fdt = NULL;
	for (int i = 0; i < kho_scratch_cnt; i++) {
		void *start = __va(kho_scratch[i].addr);
		void *end = start + kho_scratch[i].size;

		free_reserved_area(start, end, -1, "");
	}
/* …（同文件无关代码省略）… */

	/*
	 * kho_get_mem_map() should always succeed. If it fails, kho_populate()
	 * catches that and never sets kho_in.scratch_phys, which stops memory
	 * retrieval.
	 */
	mem_map = kho_get_mem_map(fdt);
	if (WARN_ON(!mem_map))
		goto err;

	/*
	 * kho_scratch_overlap() needs kho_scratch to be initialized. It
	 * is used by free_area_init() on KHO boots, so initialize it
	 * early.
	 */
	kho_scratch = phys_to_virt(kho_in.scratch_phys);

	if (kho_radix_init_tree(&kho_in.radix_tree, mem_map))
		goto err;

	kho_extend_scratch();

	return;

err:
	/*
	 * Failed to initialize preserved memory radix tree. Clear FDT
	 * and scratch so KHO users don't treat it as a KHO boot.
	 */
	kho_in.fdt_phys = 0;
	kho_in.scratch_phys = 0;
}

void __init kho_memory_init(void)
{
	if (kho_in.scratch_phys)
		kho_mem_retrieve();
	else
		kho_reserve_scratch();
}

void __init kho_populate(phys_addr_t fdt_phys, u64 fdt_len,
			 phys_addr_t scratch_phys, u64 scratch_len)
{
	unsigned int scratch_cnt = scratch_len / sizeof(*kho_scratch);
	struct kho_scratch *scratch = NULL;
	phys_addr_t mem_map_phys;
	void *fdt = NULL;
	bool populated = false;
/* …（同文件无关代码省略）… */
	if (!mem_map_phys)
		goto unmap_fdt;

	scratch = early_memremap(scratch_phys, scratch_len);
	if (!scratch) {
		pr_warn("setup: failed to memremap scratch (phys=0x%llx, len=%lld)\n",
			scratch_phys, scratch_len);
		goto unmap_fdt;
	}

	/*
	 * We pass a safe contiguous blocks of memory to use for early boot
	 * purporses from the previous kernel so that we can resize the
	 * memblock array as needed.
	 */
	for (int i = 0; i < scratch_cnt; i++) {
		struct kho_scratch *area = &scratch[i];
		u64 size = area->size;

		memblock_add(area->addr, size);
		err = memblock_mark_kho_scratch(area->addr, size);
		if (err) {
			pr_warn("failed to mark the scratch region 0x%pa+0x%pa: %pe",
				&area->addr, &size, ERR_PTR(err));
			goto unmap_scratch;
		}
		pr_debug("Marked 0x%pa+0x%pa as scratch", &area->addr, &size);
	}

	memblock_reserve(scratch_phys, scratch_len);

	/*
	 * Now that we have a viable region of scratch memory, let's tell
	 * the memblocks allocator to only use that for any allocations.
	 * That way we ensure that nothing scribbles over in use data while
	 * we initialize the page tables which we will need to ingest all
	 * memory reservations from the previous kernel.
	 */
	memblock_set_kho_scratch_only();

	kho_in.fdt_phys = fdt_phys;
	kho_in.scratch_phys = scratch_phys;
	kho_scratch_cnt = scratch_cnt;

	populated = true;
	pr_info("found kexec handover data.\n");

unmap_scratch:
	early_memunmap(scratch, scratch_len);
unmap_fdt:
	early_memunmap(fdt, fdt_len);
report:
/* …（同文件无关代码省略）… */

int kho_fill_kimage(struct kimage *image)
{
	ssize_t scratch_size;
	int err = 0;
	struct kexec_buf scratch;

	if (!kho_enable || image->type == KEXEC_TYPE_CRASH)
		return 0;

	image->kho.fdt = virt_to_phys(kho_out.fdt);

	scratch_size = sizeof(*kho_scratch) * kho_scratch_cnt;
	scratch = (struct kexec_buf){
		.image = image,
		.buffer = kho_scratch,
		.bufsz = scratch_size,
		.mem = KEXEC_BUF_MEM_UNKNOWN,
		.memsz = scratch_size,
		.buf_align = SZ_64K, /* Makes it easier to map */
		.buf_max = ULONG_MAX,
		.top_down = true,
	};
	err = kexec_add_buffer(&scratch);
	if (err)
		return err;
	image->kho.scratch = &image->segment[image->nr_segments - 1];

	return 0;
}

static int kho_walk_scratch(struct kexec_buf *kbuf,
			    int (*func)(struct resource *, void *))
{
	int ret = 0;
	int i;

	for (i = 0; i < kho_scratch_cnt; i++) {
		struct resource res = {
			.start = kho_scratch[i].addr,
			.end = kho_scratch[i].addr + kho_scratch[i].size - 1,
		};

		/* Try to fit the kimage into our KHO scratch region */
		ret = func(&res, kbuf);
		if (ret)
			break;
/* …（同文件无关代码省略）… */
	if (!kho_enable || kbuf->image->type == KEXEC_TYPE_CRASH)
		return 1;

	ret = kho_walk_scratch(kbuf, func);

	return ret == 1 ? 0 : -EADDRNOTAVAIL;
}
