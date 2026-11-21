// AUTO-DRAFT from torvalds/linux PR #b152d7176339f11f05457799be609268e1a61cf2
	gc->parent = &pdev->dev;
	gc->base = -1;
	gc->label = "ocelot-gpio";
  // <<< BUG ANCHOR
	irq = platform_get_irq_optional(pdev, 0);
	if (irq > 0) {
		girq = &gc->irq;
		gpio_irq_chip_set_chip(girq, &ocelot_irqchip);
/* …（同文件无关代码省略）… */
		.reg_bits = 32,
		.val_bits = 32,
		.reg_stride = 4,
	};

	info = devm_kzalloc_obj(dev, *info);
