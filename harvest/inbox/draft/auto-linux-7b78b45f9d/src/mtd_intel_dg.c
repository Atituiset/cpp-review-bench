// AUTO-DRAFT from torvalds/linux PR #a243ede718463c7b481878656f1ff32a0ce0fd54
#define NVM_DEBUG_REG         0x00000000
/* …（同文件无关代码省略）… */
#define NVM_ERASE_REG         0x00000048
/* …（同文件无关代码省略）… */
#define NVM_NON_POSTED_ERASE_DONE BIT(23)
#define NVM_NON_POSTED_ERASE_DONE_ITER 3000
/* …（同文件无关代码省略）… */
	void __iomem *base2 = nvm->base2;
	void __iomem *base = nvm->base;
	const u32 block = 0x10;
	u32 iter = 0;  // <<< BUG ANCHOR
	u32 reg;
	u64 i;

/* …（同文件无关代码省略）… */
		iowrite32(region << 24 | block, base + NVM_ERASE_REG);
		if (nvm->non_posted_erase) {
			/* Wait for Erase Done */
			reg = ioread32(base2 + NVM_DEBUG_REG);
			while (!(reg & NVM_NON_POSTED_ERASE_DONE) &&
			       ++iter < NVM_NON_POSTED_ERASE_DONE_ITER) {
