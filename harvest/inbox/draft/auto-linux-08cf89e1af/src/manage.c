// AUTO-DRAFT from torvalds/linux PR #2b19759b46aee85627b58f3722846424201f24a7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static int __irq_get_irqchip_state(struct irq_data *data, enum irqchip_irq_state which, bool *state)
{
	struct irq_chip *chip;
	int err = -EINVAL;

	do {
		chip = irq_data_get_irq_chip(data);
		if (WARN_ON_ONCE(!chip))
			return -ENODEV;
		if (chip->irq_get_irqchip_state)
			break;
		data = irqd_get_parent_data(data);
	} while (data);

	if (data)
		err = chip->irq_get_irqchip_state(data, which, state);
	return err;
}
/* …（同文件无关代码省略）… */
int irq_get_irqchip_state(unsigned int irq, enum irqchip_irq_state which, bool *state)
{
	scoped_irqdesc_get_and_buslock(irq, 0) {
		struct irq_data *data = irq_desc_get_irq_data(scoped_irqdesc);

		return __irq_get_irqchip_state(data, which, state);
	}
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(irq_get_irqchip_state);

/**
 * irq_set_irqchip_state - set the state of a forwarded interrupt.
 * @irq:	Interrupt line that is forwarded to a VM
/* …（同文件无关代码省略）… */
 */
int irq_set_irqchip_state(unsigned int irq, enum irqchip_irq_state which, bool val)
{
	scoped_irqdesc_get_and_buslock(irq, 0) {
		struct irq_data *data = irq_desc_get_irq_data(scoped_irqdesc);
		struct irq_chip *chip;

		do {
			chip = irq_data_get_irq_chip(data);

			if (WARN_ON_ONCE(!chip))
				return -ENODEV;

			if (chip->irq_set_irqchip_state)
				break;

			data = irqd_get_parent_data(data);
		} while (data);

		if (data)
			return chip->irq_set_irqchip_state(data, which, val);
	}
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(irq_set_irqchip_state);
