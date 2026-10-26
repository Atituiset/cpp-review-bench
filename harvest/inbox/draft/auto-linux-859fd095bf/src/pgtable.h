// AUTO-DRAFT from torvalds/linux PR #6b4b543fa18757297ac756a6fe6e64f506b7844a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define _PAGE_WRITE_BIT    30   /* (0x002) write access allowed */
/* …（同文件无关代码省略）… */
#define _PAGE_ACCESSED_BIT 23   /* (0x100) Software: Page Accessed */
/* …（同文件无关代码省略）… */
#define xlate_pabit(x) (31 - x)
/* …（同文件无关代码省略）… */
#define _PAGE_WRITE    (1 << xlate_pabit(_PAGE_WRITE_BIT))
/* …（同文件无关代码省略）… */
#define _PAGE_ACCESSED (1 << xlate_pabit(_PAGE_ACCESSED_BIT))
/* …（同文件无关代码省略）… */
static inline int pte_young(pte_t pte)		{ return pte_val(pte) & _PAGE_ACCESSED; }
/* …（同文件无关代码省略）… */
static inline pte_t pte_mkold(pte_t pte)	{ pte_val(pte) &= ~_PAGE_ACCESSED; return pte; }
static inline pte_t pte_wrprotect(pte_t pte)	{ pte_val(pte) &= ~_PAGE_WRITE; return pte; }
/* …（同文件无关代码省略）… */
#define set_ptes set_ptes
#define set_pte_at(mm, addr, ptep, pte) set_ptes(mm, addr, ptep, pte, 1)
/* …（同文件无关代码省略）… */
	return pte;
}

static inline pte_t ptep_get(pte_t *ptep)
{
	return READ_ONCE(*ptep);
}
#define ptep_get ptep_get

static inline bool ptep_test_and_clear_young(struct vm_area_struct *vma,
		unsigned long addr, pte_t *ptep)
{
	pte_t pte;

	pte = ptep_get(ptep);
	if (!pte_young(pte)) {
		return false;
	}
	set_pte_at(vma->vm_mm, addr, ptep, pte_mkold(pte));
	return true;
}

bool ptep_clear_flush_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep);
pte_t ptep_clear_flush(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep);

struct mm_struct;
static inline void ptep_set_wrprotect(struct mm_struct *mm, unsigned long addr, pte_t *ptep)
{
	set_pte_at(mm, addr, ptep, pte_wrprotect(*ptep));
}

#define pte_same(A,B)	(pte_val(A) == pte_val(B))

#endif /* !__ASSEMBLER__ */


/* …（同文件无关代码省略）… */
#define HAVE_ARCH_UNMAPPED_AREA
#define HAVE_ARCH_UNMAPPED_AREA_TOPDOWN

#define __HAVE_ARCH_PTEP_TEST_AND_CLEAR_YOUNG
#define __HAVE_ARCH_PTEP_CLEAR_YOUNG_FLUSH
#define __HAVE_ARCH_PTEP_CLEAR_FLUSH
#define __HAVE_ARCH_PTEP_SET_WRPROTECT
#define __HAVE_ARCH_PTE_SAME

#endif /* _PARISC_PGTABLE_H */
