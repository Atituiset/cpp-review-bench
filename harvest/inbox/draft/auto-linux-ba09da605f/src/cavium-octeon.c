// AUTO-DRAFT from torvalds/linux PR #b71ccf604fbdf78450d3b17b5d69365caba29061
	u64 dma_cfg;
	int i;
  // <<< BUG ANCHOR
	for (i = 0; i < CAVIUM_MAX_MMC; i++)
		if (host->slot[i])
			cvm_mmc_of_slot_remove(host->slot[i]);

	dma_cfg = readq(host->dma_base + MIO_EMM_DMA_CFG(host));
	dma_cfg &= ~MIO_EMM_DMA_CFG_EN;
