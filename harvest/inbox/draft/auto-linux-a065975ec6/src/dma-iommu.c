// AUTO-DRAFT from torvalds/linux PR #906b6e1d8c782a8442b30bdf8b8992f5031ffc59
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	void *cpu_addr;

	page = dma_alloc_contiguous(dev, alloc_size, gfp);
	if (!page)
		page = alloc_pages_node(node, gfp, get_order(alloc_size));
	if (!page)
		return NULL;

	if (!coherent || PageHighMem(page)) {
		pgprot_t prot = dma_pgprot(dev, PAGE_KERNEL, attrs);
/* …（同文件无关代码省略）… */
	return (1UL << __ffs(domain->pgsize_bitmap)) - 1;
}

size_t iommu_dma_opt_mapping_size(void)
{
	return iova_rcache_range();
}
