// AUTO-DRAFT from torvalds/linux PR #42e225323067a222230068651cf886bd7fbb1fe2
	return (cur - base) >> PAGE_SHIFT;
}
  // <<< BUG ANCHOR
void *hyp_early_alloc_contig(unsigned int nr_pages)
{
	unsigned long size = (nr_pages << PAGE_SHIFT);
	void *ret = (void *)cur;
