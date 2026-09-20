// AUTO-DRAFT from torvalds/linux PR #786262be6048deab760f68c8acc2c85607165894
	struct irq_chip		irq_chip;
	struct list_head	a10_ecc_devices;
	struct notifier_block	panic_notifier;
};

#endif	/* #ifndef _ALTERA_EDAC_H */
