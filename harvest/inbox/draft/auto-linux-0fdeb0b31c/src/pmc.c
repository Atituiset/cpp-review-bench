// AUTO-DRAFT from torvalds/linux PR #2f2f7eeef8b0a1a9b9ca998231419873ccae8d4e

#if defined(CONFIG_ARM)
	mutex_lock(&early_pmc->powergates_lock);
	iounmap(early_pmc->base);
	early_pmc->base = pmc->base;
	mutex_unlock(&early_pmc->powergates_lock);
