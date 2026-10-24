// AUTO-DRAFT from torvalds/linux PR #c5fd5e67c7812bc40cd897422c1e0d6dd9ed1e2e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
* Alignments up to the page size are the same for physical and virtual
 * addresses of the nocache area.
 */
pgtable_t pte_alloc_one(struct mm_struct *mm)
{
	pte_t *ptep;
	struct page *page;

	if (!(ptep = pte_alloc_one_kernel(mm)))
		return NULL;
	page = pfn_to_page(__nocache_pa((unsigned long)ptep) >> PAGE_SHIFT);
	spin_lock(&mm->page_table_lock);
	if (page_ref_inc_return(page) == 2 &&
			!pagetable_pte_ctor(mm, page_ptdesc(page))) {
		page_ref_dec(page);
		ptep = NULL;
	}
	spin_unlock(&mm->page_table_lock);

	return ptep;
}

void pte_free(struct mm_struct *mm, pgtable_t ptep)
{
	struct page *page;

	page = pfn_to_page(__nocache_pa((unsigned long)ptep) >> PAGE_SHIFT);
	spin_lock(&mm->page_table_lock);
	if (page_ref_dec_return(page) == 1)
		pagetable_dtor(page_ptdesc(page));
	spin_unlock(&mm->page_table_lock);

	srmmu_free_nocache(ptep, SRMMU_PTE_TABLE_SIZE);
}

/* context handling - a dynamically sized pool is used */
#define NO_CONTEXT	-1
