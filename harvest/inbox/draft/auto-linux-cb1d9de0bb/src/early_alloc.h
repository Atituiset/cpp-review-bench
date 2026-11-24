// AUTO-DRAFT from torvalds/linux PR #42e225323067a222230068651cf886bd7fbb1fe2
void hyp_early_alloc_init(void *virt, unsigned long size);
unsigned long hyp_early_alloc_nr_used_pages(void);
void *hyp_early_alloc_page(void *arg);
void *hyp_early_alloc_contig(unsigned int nr_pages);  // <<< BUG ANCHOR

extern struct kvm_pgtable_mm_ops hyp_early_alloc_mm_ops;
