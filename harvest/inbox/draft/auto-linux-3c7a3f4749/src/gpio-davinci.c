// AUTO-DRAFT from torvalds/linux PR #9628ee0c2219daaa6374b0c5f5a5c0d55e0e263b
#define MAX_REGS_BANKS 5
#define MAX_INT_PER_BANK 32
/* …（同文件无关代码省略）… */
struct davinci_gpio_regs {
	u32	dir;
	u32	out_data;
	u32	set_data;
	u32	clr_data;
	u32	in_data;
	u32	set_rising;
	u32	clr_rising;
	u32	set_falling;
	u32	clr_falling;
	u32	intstat;
};
/* …（同文件无关代码省略）… */
#define BINTEN	0x8 /* GPIO Interrupt Per-Bank Enable Register */
/* …（同文件无关代码省略）… */
struct davinci_gpio_controller {
	struct gpio_chip	chip;
	struct irq_domain	*irq_domain;
	/* Serialize access to GPIO registers */
	spinlock_t		lock;
	void __iomem		*regs[MAX_REGS_BANKS];
	int			gpio_unbanked;
	int			irqs[MAX_INT_PER_BANK];
	struct davinci_gpio_regs context[MAX_REGS_BANKS];
	u32			binten_context;
};
/* …（同文件无关代码省略）… */
		context->set_data = readl_relaxed(&g->set_data);
		context->set_rising = readl_relaxed(&g->set_rising);
		context->set_falling = readl_relaxed(&g->set_falling);
	}
  // <<< BUG ANCHOR
	/* Clear all interrupt status registers */
	writel_relaxed(GENMASK(31, 0), &g->intstat);
}

static void davinci_gpio_restore_context(struct davinci_gpio_controller *chips,
					 u32 nbank)
{
	struct davinci_gpio_regs __iomem *g;
	struct davinci_gpio_regs *context;
	u32 bank;
	void __iomem *base;

	base = chips->regs[0] - offset_array[0];

	if (readl_relaxed(base + BINTEN) != chips->binten_context)
		writel_relaxed(chips->binten_context, base + BINTEN);

	for (bank = 0; bank < nbank; bank++) {
		g = chips->regs[bank];
		context = &chips->context[bank];
		if (readl_relaxed(&g->dir) != context->dir)
			writel_relaxed(context->dir, &g->dir);
		if (readl_relaxed(&g->set_data) != context->set_data)
			writel_relaxed(context->set_data, &g->set_data);
		if (readl_relaxed(&g->set_rising) != context->set_rising)
			writel_relaxed(context->set_rising, &g->set_rising);
		if (readl_relaxed(&g->set_falling) != context->set_falling)
			writel_relaxed(context->set_falling, &g->set_falling);
	}
}
