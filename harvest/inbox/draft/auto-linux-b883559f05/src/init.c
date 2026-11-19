// AUTO-DRAFT from torvalds/linux PR #d866d5bafc5e1417032255002e95f8ec3121754b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
  // <<< BUG ANCHOR
#endif /* CONFIG_EXECMEM */

#ifdef CONFIG_MEMORY_HOTPLUG
static void __meminit free_pte_table(pte_t *pte_start, pmd_t *pmd)
{
	struct page *page = pmd_page(*pmd);
	struct ptdesc *ptdesc = page_ptdesc(page);
	pte_t *pte;
	int i;

/* …（同文件无关代码省略）… */
			return;
	}

	pagetable_dtor(ptdesc);
	if (PageReserved(page))
		free_reserved_page(page);
	else
		pagetable_free(ptdesc);
	pmd_clear(pmd);
}

static void __meminit free_pmd_table(pmd_t *pmd_start, pud_t *pud, bool is_vmemmap)
{
	struct page *page = pud_page(*pud);
	struct ptdesc *ptdesc = page_ptdesc(page);
	pmd_t *pmd;
	int i;

/* …（同文件无关代码省略）… */
			return;
	}

	if (!is_vmemmap)
		pagetable_dtor(ptdesc);
	if (PageReserved(page))
		free_reserved_page(page);
	else
		pagetable_free(ptdesc);
	pud_clear(pud);
}

/* …（同文件无关代码省略）… */
			return;
	}

	if (PageReserved(page))
		free_reserved_page(page);
	else
		__free_pages(page, 0);
	p4d_clear(p4d);
}

static void __meminit free_vmemmap_storage(struct page *page, size_t size,
					   struct vmem_altmap *altmap)
{
	int order = get_order(size);

	if (altmap) {
		vmem_altmap_free(altmap, size >> PAGE_SHIFT);
		return;
	}

	if (PageReserved(page)) {
		unsigned int nr_pages = 1 << order;

		while (nr_pages--)
			free_reserved_page(page++);
		return;
	}

	__free_pages(page, order);
}
/* …（同文件无关代码省略）… */
static void __meminit remove_pte_mapping(pte_t *pte_base, unsigned long addr, unsigned long end,
					 bool is_vmemmap, struct vmem_altmap *altmap)
{
	unsigned long next;
	pte_t *ptep, pte;

	for (; addr < end; addr = next) {
		next = (addr + PAGE_SIZE) & PAGE_MASK;
		if (next > end)
			next = end;

		ptep = pte_base + pte_index(addr);
		pte = ptep_get(ptep);
		if (!pte_present(*ptep))
			continue;

		pte_clear(&init_mm, addr, ptep);
		if (is_vmemmap)
			free_vmemmap_storage(pte_page(pte), PAGE_SIZE, altmap);
	}
}
/* …（同文件无关代码省略）… */
static void __meminit remove_pmd_mapping(pmd_t *pmd_base, unsigned long addr, unsigned long end,
					 bool is_vmemmap, struct vmem_altmap *altmap)
{
	unsigned long next;
	pte_t *pte_base;
	pmd_t *pmdp, pmd;

	for (; addr < end; addr = next) {
		next = pmd_addr_end(addr, end);
		pmdp = pmd_base + pmd_index(addr);
		pmd = pmdp_get(pmdp);
		if (!pmd_present(pmd))
			continue;

		if (pmd_leaf(pmd)) {
			pmd_clear(pmdp);
			if (is_vmemmap)
				free_vmemmap_storage(pmd_page(pmd), PMD_SIZE, altmap);
			continue;
		}

		pte_base = (pte_t *)pmd_page_vaddr(*pmdp);
		remove_pte_mapping(pte_base, addr, next, is_vmemmap, altmap);
		free_pte_table(pte_base, pmdp);
	}
}
/* …（同文件无关代码省略）… */
		remove_pmd_mapping(pmd_base, addr, next, is_vmemmap, altmap);

		if (pgtable_l4_enabled())
			free_pmd_table(pmd_base, pudp, is_vmemmap);
	}
}
