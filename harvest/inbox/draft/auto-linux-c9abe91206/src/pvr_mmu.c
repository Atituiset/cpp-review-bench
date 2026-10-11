// AUTO-DRAFT from torvalds/linux PR #c61e23ed14359e6289c08e55940dd3401fe27db0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
enum pvr_mmu_sync_level {
	PVR_MMU_SYNC_LEVEL_NONE = -1,
	PVR_MMU_SYNC_LEVEL_0 = 0,
	PVR_MMU_SYNC_LEVEL_1 = 1,
	PVR_MMU_SYNC_LEVEL_2 = 2,
};
/* …（同文件无关代码省略）… */
struct pvr_mmu_backing_page {
	dma_addr_t dma_addr;
	void *host_ptr;
/* private: internal use only */
	struct page *raw_page;
	struct pvr_device *pvr_dev;
};
/* …（同文件无关代码省略）… */
struct pvr_page_table_l2 {
	/**
	 * @entries: The children of this node in the page table tree
	 * structure. These are also mirror tables. The indexing of this array
	 * is identical to that of the raw equivalent
	 * (&pvr_page_table_l1_raw.entries).
	 */
	struct pvr_page_table_l1 *entries[ROGUE_MMUCTRL_ENTRIES_PC_VALUE];

	/**
	 * @backing_page: A handle to the memory which holds the raw
	 * equivalent of this table. **For internal use only.**
	 */
	struct pvr_mmu_backing_page backing_page;

	/**
	 * @entry_count: The current number of valid entries (that we know of)
	 * in this table. This value is essentially a refcount - the table is
	 * destroyed when this value is decremented to zero by
	 * pvr_page_table_l2_remove().
	 */
	u16 entry_count;
};
/* …（同文件无关代码省略）… */
struct pvr_page_table_l1 {
	/**
	 * @entries: The children of this node in the page table tree
	 * structure. These are also mirror tables. The indexing of this array
	 * is identical to that of the raw equivalent
	 * (&pvr_page_table_l0_raw.entries).
	 */
	struct pvr_page_table_l0 *entries[ROGUE_MMUCTRL_ENTRIES_PD_VALUE];

	/**
	 * @backing_page: A handle to the memory which holds the raw
	 * equivalent of this table. **For internal use only.**
	 */
	struct pvr_mmu_backing_page backing_page;

	union {
		/**
		 * @parent: The parent of this node in the page table tree structure.
		 *
		 * This is also a mirror table.
		 *
		 * Only valid when the L1 page table is active. When the L1 page table
		 * has been removed and queued for destruction, the next_free field
		 * should be used instead.
		 */
		struct pvr_page_table_l2 *parent;

		/**
		 * @next_free: Pointer to the next L1 page table to take/free.
		 *
		 * Used to form a linked list of L1 page tables. This is used
		 * when preallocating tables and when the page table has been
		 * removed and queued for destruction.
		 */
		struct pvr_page_table_l1 *next_free;
	};

	/**
	 * @parent_idx: The index of the entry in the parent table (see
	 * @parent) which corresponds to this table.
	 */
	u16 parent_idx;

	/**
	 * @entry_count: The current number of valid entries (that we know of)
	 * in this table. This value is essentially a refcount - the table is
	 * destroyed when this value is decremented to zero by
	 * pvr_page_table_l1_remove().
	 */
	u16 entry_count;
};
/* …（同文件无关代码省略）… */
struct pvr_page_table_l0 {
	/**
	 * @backing_page: A handle to the memory which holds the raw
	 * equivalent of this table. **For internal use only.**
	 */
	struct pvr_mmu_backing_page backing_page;

	union {
		/**
		 * @parent: The parent of this node in the page table tree structure.
		 *
		 * This is also a mirror table.
		 *
		 * Only valid when the L0 page table is active. When the L0 page table
		 * has been removed and queued for destruction, the next_free field
		 * should be used instead.
		 */
		struct pvr_page_table_l1 *parent;

		/**
		 * @next_free: Pointer to the next L0 page table to take/free.
		 *
		 * Used to form a linked list of L0 page tables. This is used
		 * when preallocating tables and when the page table has been
		 * removed and queued for destruction.
		 */
		struct pvr_page_table_l0 *next_free;
	};

	/**
	 * @parent_idx: The index of the entry in the parent table (see
	 * @parent) which corresponds to this table.
	 */
	u16 parent_idx;

	/**
	 * @entry_count: The current number of valid entries (that we know of)
	 * in this table. This value is essentially a refcount - the table is
	 * destroyed when this value is decremented to zero by
	 * pvr_page_table_l0_remove().
	 */
	u16 entry_count;
};
/* …（同文件无关代码省略）… */
struct pvr_mmu_context {
	/** @pvr_dev: The PVR device associated with the owning VM context. */
	struct pvr_device *pvr_dev;

	/** @page_table_l2: The MMU table root. */
	struct pvr_page_table_l2 page_table_l2;
};
/* …（同文件无关代码省略）… */
struct pvr_page_table_ptr {
	/**
	 * @l1_table: A cached handle to the level 1 page table the
	 * context is currently traversing.
	 */
	struct pvr_page_table_l1 *l1_table;

	/**
	 * @l0_table: A cached handle to the level 0 page table the
	 * context is currently traversing.
	 */
	struct pvr_page_table_l0 *l0_table;

	/**
	 * @l2_idx: Index into the level 2 page table the context is
	 * currently referencing.
	 */
	u16 l2_idx;

	/**
	 * @l1_idx: Index into the level 1 page table the context is
	 * currently referencing.
	 */
	u16 l1_idx;

	/**
	 * @l0_idx: Index into the level 0 page table the context is
	 * currently referencing.
	 */
	u16 l0_idx;
};
/* …（同文件无关代码省略）… */
struct pvr_mmu_op_context {
	/** @mmu_ctx: The MMU context associated with the owning VM context. */
	struct pvr_mmu_context *mmu_ctx;

	/** @map: Data specifically for map operations. */
	struct {
		/**
		 * @sgt: Scatter gather table containing pages pinned for use by
		 * this context - these are currently pinned when initialising
		 * the VM bind operation.
		 */
		struct sg_table *sgt;

		/** @sgt_offset: Start address of the device-virtual mapping. */
		u64 sgt_offset;

		/**
		 * @l1_prealloc_tables: Preallocated l1 page table objects
		 * use by this context when creating a page mapping. Linked list
		 * fully created during initialisation.
		 */
		struct pvr_page_table_l1 *l1_prealloc_tables;

		/**
		 * @l0_prealloc_tables: Preallocated l0 page table objects
		 * use by this context when creating a page mapping. Linked list
		 * fully created during initialisation.
		 */
		struct pvr_page_table_l0 *l0_prealloc_tables;
	} map;

	/** @unmap: Data specifically for unmap operations. */
	struct {
		/**
		 * @l1_free_tables: Collects page table objects freed by unmap
		 * ops. Linked list empty at creation.
		 */
		struct pvr_page_table_l1 *l1_free_tables;

		/**
		 * @l0_free_tables: Collects page table objects freed by unmap
		 * ops. Linked list empty at creation.
		 */
		struct pvr_page_table_l0 *l0_free_tables;
	} unmap;

	/**
	 * @curr_page: A reference to a single physical page as indexed by the
	 * page table structure.
	 */
	struct pvr_page_table_ptr curr_page;

	/**
	 * @sync_level_required: The maximum level of the page table tree
	 * structure which has (possibly) been modified since it was last
	 * flushed to the device.
	 *
	 * This field should only be set with pvr_mmu_op_context_require_sync()
	 * or indirectly by pvr_mmu_op_context_sync_partial().
	 */
	enum pvr_mmu_sync_level sync_level_required;
};
/* …（同文件无关代码省略）… */
static u16
pvr_page_table_l2_idx(u64 device_addr)
{
	return (device_addr & ~ROGUE_MMUCTRL_VADDR_PC_INDEX_CLRMSK) >>
	       ROGUE_MMUCTRL_VADDR_PC_INDEX_SHIFT;
}
/* …（同文件无关代码省略）… */
static u16
pvr_page_table_l1_idx(u64 device_addr)
{
	return (device_addr & ~ROGUE_MMUCTRL_VADDR_PD_INDEX_CLRMSK) >>
	       ROGUE_MMUCTRL_VADDR_PD_INDEX_SHIFT;
}
/* …（同文件无关代码省略）… */
 * @ctx: MMU context associated with owning VM context.
 * @sgt: Scatter gather table containing pages pinned for use by this context.
 * @device_addr: Virtual device address at the start of the requested mapping.
 * @sgt_offset: Start offset of the requested device-virtual memory mapping.
 * @size: Size in bytes of the requested device-virtual memory mapping. For an
 * unmapping, this should be zero so that no page tables are allocated.
 *
/* …（同文件无关代码省略）… */
pvr_mmu_op_context_create(struct pvr_mmu_context *ctx, struct sg_table *sgt,
			  u64 device_addr, u64 sgt_offset, u64 size)
{
	u64 start_addr = device_addr + sgt_offset;
	int err;

	struct pvr_mmu_op_context *op_ctx = kzalloc_obj(*op_ctx);
/* …（同文件无关代码省略）… */

	if (size) {
		/*
		 * The number of page table objects we need to prealloc is
		 * indicated by the mapping size, start address and the sizes
		 * of the areas mapped per PT or PD. The range calculation is
		 * identical to that for the index into a table for a device
		 * address, so we reuse those functions here.
		 */
		const u32 l1_start_idx = pvr_page_table_l2_idx(start_addr);
		const u32 l1_end_idx = pvr_page_table_l2_idx(start_addr + size);
		const u32 l1_count = l1_end_idx - l1_start_idx + 1;
		const u32 l0_start_idx = pvr_page_table_l1_idx(start_addr);
		const u32 l0_end_idx = pvr_page_table_l1_idx(start_addr + size);
		const u32 l0_count = l0_end_idx - l0_start_idx + 1;

		/*
		 * Alloc and push page table entries until we have enough of
