// AUTO-DRAFT from torvalds/linux PR #9628ee0c2219daaa6374b0c5f5a5c0d55e0e263b
  // <<< BUG ANCHOR
static int lpc32xx_gpio_probe(struct platform_device *pdev)
{
	int i;
	void __iomem *reg_base;

	reg_base = devm_platform_ioremap_resource(pdev, 0);
/* …（同文件无关代码省略）… */
			lpc32xx_gpiochip[i].chip.of_gpio_n_cells = 3;
			lpc32xx_gpiochip[i].reg_base = reg_base;
		}
		devm_gpiochip_add_data(&pdev->dev, &lpc32xx_gpiochip[i].chip,
				  &lpc32xx_gpiochip[i]);
	}

	return 0;
