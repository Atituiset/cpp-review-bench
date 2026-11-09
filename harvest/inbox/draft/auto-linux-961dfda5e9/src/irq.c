// AUTO-DRAFT from torvalds/linux PR #2651f6deb3326129a052897df193052dc28ea94f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define IC_CFG0SET	0x40
#define IC_CFG0CLR	0x44
/* …（同文件无关代码省略）… */
#define IC_CFG1SET	0x48
#define IC_CFG1CLR	0x4C
/* …（同文件无关代码省略）… */
#define IC_CFG2SET	0x50
#define IC_CFG2CLR	0x54
/* …（同文件无关代码省略）… */
static int au1x_ic_settype(struct irq_data *d, unsigned int flow_type)
{
	struct irq_chip *chip;
	unsigned int bit, irq = d->irq;
	irq_flow_handler_t handler = NULL;
	unsigned char *name = NULL;
	void __iomem *base;
	int ret;

	if (irq >= AU1000_INTC1_INT_BASE) {
		bit = irq - AU1000_INTC1_INT_BASE;
		chip = &au1x_ic1_chip;
		base = (void __iomem *)KSEG1ADDR(AU1000_IC1_PHYS_ADDR);
	} else {
		bit = irq - AU1000_INTC0_INT_BASE;
		chip = &au1x_ic0_chip;
		base = (void __iomem *)KSEG1ADDR(AU1000_IC0_PHYS_ADDR);
	}

	if (bit > 31)
		return -EINVAL;

	ret = 0;

	switch (flow_type) {	/* cfgregs 2:1:0 */
	case IRQ_TYPE_EDGE_RISING:	/* 0:0:1 */
		__raw_writel(1 << bit, base + IC_CFG2CLR);
		__raw_writel(1 << bit, base + IC_CFG1CLR);
		__raw_writel(1 << bit, base + IC_CFG0SET);
		handler = handle_edge_irq;
		name = "riseedge";
		break;
	case IRQ_TYPE_EDGE_FALLING:	/* 0:1:0 */
		__raw_writel(1 << bit, base + IC_CFG2CLR);
		__raw_writel(1 << bit, base + IC_CFG1SET);
		__raw_writel(1 << bit, base + IC_CFG0CLR);
		handler = handle_edge_irq;
		name = "falledge";
		break;
	case IRQ_TYPE_EDGE_BOTH:	/* 0:1:1 */
		__raw_writel(1 << bit, base + IC_CFG2CLR);
		__raw_writel(1 << bit, base + IC_CFG1SET);
		__raw_writel(1 << bit, base + IC_CFG0SET);
		handler = handle_edge_irq;
		name = "bothedge";
		break;
	case IRQ_TYPE_LEVEL_HIGH:	/* 1:0:1 */
		__raw_writel(1 << bit, base + IC_CFG2SET);
		__raw_writel(1 << bit, base + IC_CFG1CLR);
		__raw_writel(1 << bit, base + IC_CFG0SET);
		handler = handle_level_irq;
		name = "hilevel";
		break;
	case IRQ_TYPE_LEVEL_LOW:	/* 1:1:0 */
		__raw_writel(1 << bit, base + IC_CFG2SET);
		__raw_writel(1 << bit, base + IC_CFG1SET);
		__raw_writel(1 << bit, base + IC_CFG0CLR);
		handler = handle_level_irq;
		name = "lowlevel";
		break;
	case IRQ_TYPE_NONE:		/* 0:0:0 */
		__raw_writel(1 << bit, base + IC_CFG2CLR);
		__raw_writel(1 << bit, base + IC_CFG1CLR);
		__raw_writel(1 << bit, base + IC_CFG0CLR);
		break;
	default:
		ret = -EINVAL;
	}
	irq_set_chip_handler_name_locked(d, chip, handler, name);

	wmb();

	return ret;
}
/* …（同文件无关代码省略）… */
static inline void au1300_gpic_chgcfg(unsigned int gpio,
				      unsigned long clr,
				      unsigned long set)
{
	void __iomem *r = AU1300_GPIC_ADDR;
	unsigned long l;

	r += gpio * 4;	/* offset into pin config array */
	l = __raw_readl(r + AU1300_GPIC_PINCFG);
	l &= ~clr;
	l |= set;
	__raw_writel(l, r + AU1300_GPIC_PINCFG);
	wmb();
}
/* …（同文件无关代码省略）… */
void au1300_set_irq_priority(unsigned int irq, int p)
{
	irq -= ALCHEMY_GPIC_INT_BASE;
	au1300_gpic_chgcfg(irq, GPIC_CFG_IL_MASK, GPIC_CFG_IL_SET(p));
}
/* …（同文件无关代码省略）… */
static int au1300_gpic_settype(struct irq_data *d, unsigned int type)
{
	unsigned long s;
	unsigned char *name = NULL;
	irq_flow_handler_t hdl = NULL;

	switch (type) {
	case IRQ_TYPE_LEVEL_HIGH:
		s = GPIC_CFG_IC_LEVEL_HIGH;
		name = "high";
		hdl = handle_level_irq;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		s = GPIC_CFG_IC_LEVEL_LOW;
		name = "low";
		hdl = handle_level_irq;
		break;
	case IRQ_TYPE_EDGE_RISING:
		s = GPIC_CFG_IC_EDGE_RISE;
		name = "posedge";
		hdl = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		s = GPIC_CFG_IC_EDGE_FALL;
		name = "negedge";
		hdl = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		s = GPIC_CFG_IC_EDGE_BOTH;
		name = "bothedge";
		hdl = handle_edge_irq;
		break;
	case IRQ_TYPE_NONE:
		s = GPIC_CFG_IC_OFF;
		name = "disabled";
		hdl = handle_level_irq;
		break;
	default:
		return -EINVAL;
	}

	irq_set_chip_handler_name_locked(d, &au1300_gpic, hdl, name);

	au1300_gpic_chgcfg(d->irq - ALCHEMY_GPIC_INT_BASE, GPIC_CFG_IC_MASK, s);

	return 0;
}
/* …（同文件无关代码省略）… */
	mips_cpu_irq_init();

	/* register all 64 possible IC0+IC1 irq sources as type "none".
	 * Use set_irq_type() to set edge/level behaviour at runtime.
	 */
	for (irq_nr = AU1000_INTC0_INT_BASE;
	     (irq_nr < AU1000_INTC0_INT_BASE + 32); irq_nr++)
		au1x_ic_settype(irq_get_irq_data(irq_nr), IRQ_TYPE_NONE);

	for (irq_nr = AU1000_INTC1_INT_BASE;
	     (irq_nr < AU1000_INTC1_INT_BASE + 32); irq_nr++)
		au1x_ic_settype(irq_get_irq_data(irq_nr), IRQ_TYPE_NONE);

	/*
	 * Initialize IC0, which is fixed per processor.
/* …（同文件无关代码省略）… */

	/* register an irq_chip for them, with 2nd highest priority */
	for (i = ALCHEMY_GPIC_INT_BASE; i <= ALCHEMY_GPIC_INT_LAST; i++) {
		au1300_set_irq_priority(i, 1);
		au1300_gpic_settype(irq_get_irq_data(i), IRQ_TYPE_NONE);
	}
