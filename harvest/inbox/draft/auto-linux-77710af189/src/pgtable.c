// AUTO-DRAFT from torvalds/linux PR #06ac073129191a01d77933ed381f89c67674db6f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
}

#ifdef CONFIG_X86_64
/**
 * pud_free_pmd_page - Clear PUD entry and free PMD page
 * @pud: Pointer to a PUD
 * @addr: Virtual address associated with PUD
 *
 * Context: The PUD range has been unmapped and TLB purged.
 * Return: 1 if clearing the entry succeeded. 0 otherwise.
 *
 * NOTE: Callers must allow a single page allocation.
 */
int pud_free_pmd_page(pud_t *pud, unsigned long addr)
{
	pmd_t *pmd, *pmd_sv;
	struct ptdesc *pt;
	int i;

	pmd = pud_pgtable(*pud);
	pmd_sv = (pmd_t *)__get_free_page(GFP_KERNEL);
	if (!pmd_sv)
		return 0;

	for (i = 0; i < PTRS_PER_PMD; i++) {
		pmd_sv[i] = pmd[i];
		if (!pmd_none(pmd[i]))
			pmd_clear(&pmd[i]);
	}

	pud_clear(pud);

	/* INVLPG to clear all paging-structure caches */
	flush_tlb_kernel_range(addr, addr + PAGE_SIZE-1);

	for (i = 0; i < PTRS_PER_PMD; i++) {
		if (!pmd_none(pmd_sv[i])) {
			pt = page_ptdesc(pmd_page(pmd_sv[i]));
			pagetable_dtor_free(pt);
		}
	}

	free_page((unsigned long)pmd_sv);

	pmd_free(&init_mm, pmd);

	return 1;
