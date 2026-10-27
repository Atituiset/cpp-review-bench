// AUTO-DRAFT from torvalds/linux PR #d135ab453a7400a34930bdda1ef4bdc8acfac80e

	if (pchip->irq) {
		free_irq(pchip->irq, pchip);
		destroy_workqueue(pchip->irqthread);
	}
}
