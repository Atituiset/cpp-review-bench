// AUTO-DRAFT from torvalds/linux PR #20eef29b7c8465ac98d7dd6d6f2ac07a8d8056ca
#define __raw_writew __raw_writew
static inline void __raw_writew(u16 s, volatile void __iomem *addr)
{
	__asm__ __volatile__(
	"	stw%U1 %0, %1	\n"  // <<< BUG ANCHOR
	:
	: "r" (s), "m" (*(volatile u16 __force *)addr)
	: "memory");

}

#define __raw_writel __raw_writel
static inline void __raw_writel(u32 w, volatile void __iomem *addr)
{
	__asm__ __volatile__(
	"	st%U1 %0, %1	\n"
	:
	: "r" (w), "m" (*(volatile u32 __force *)addr)
	: "memory");

}
