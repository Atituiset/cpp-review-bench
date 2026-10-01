// AUTO-DRAFT from torvalds/linux PR #551c722f40809618230001baccf219193e22fc5a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define AC100_RTC_CTRL_24HOUR	BIT(0)
/* …（同文件无关代码省略）… */
#define AC100_ALM_INT_ENABLE	BIT(0)
/* …（同文件无关代码省略）… */
struct ac100_clkout {
	struct clk_hw hw;
	struct regmap *regmap;
	u8 offset;
};
/* …（同文件无关代码省略）… */
#define AC100_RTC_32K_NAME	"ac100-rtc-32k"
#define AC100_RTC_32K_RATE	32768
#define AC100_CLKOUT_NUM	3
/* …（同文件无关代码省略）… */
struct ac100_rtc_dev {
	struct rtc_device *rtc;
	struct device *dev;
	struct regmap *regmap;
	int irq;
	unsigned long alarm;

	struct clk_hw *rtc_32k_clk;
	struct ac100_clkout clks[AC100_CLKOUT_NUM];
	struct clk_hw_onecell_data *clk_data;
};
/* …（同文件无关代码省略）… */
static int ac100_rtc_register_clks(struct ac100_rtc_dev *chip)
{
	struct device_node *np = chip->dev->of_node;
	const char *parents[2] = {AC100_RTC_32K_NAME};
	int i, ret;

	chip->clk_data = devm_kzalloc(chip->dev,
				      struct_size(chip->clk_data, hws,
						  AC100_CLKOUT_NUM),
				      GFP_KERNEL);
	if (!chip->clk_data)
		return -ENOMEM;

	chip->rtc_32k_clk = clk_hw_register_fixed_rate(chip->dev,
						       AC100_RTC_32K_NAME,
						       NULL, 0,
						       AC100_RTC_32K_RATE);
	if (IS_ERR(chip->rtc_32k_clk)) {
		ret = PTR_ERR(chip->rtc_32k_clk);
		dev_err(chip->dev, "Failed to register RTC-32k clock: %d\n",
			ret);
		return ret;
	}

	parents[1] = of_clk_get_parent_name(np, 0);
	if (!parents[1]) {
		dev_err(chip->dev, "Failed to get ADDA 4M clock\n");
		return -EINVAL;
	}

	for (i = 0; i < AC100_CLKOUT_NUM; i++) {
		struct ac100_clkout *clk = &chip->clks[i];
		struct clk_init_data init = {
			.name = ac100_clkout_names[i],
			.ops = &ac100_clkout_ops,
			.parent_names = parents,
			.num_parents = ARRAY_SIZE(parents),
			.flags = 0,
		};

		of_property_read_string_index(np, "clock-output-names",
					      i, &init.name);
		clk->regmap = chip->regmap;
		clk->offset = AC100_CLKOUT_CTRL1 + i;
		clk->hw.init = &init;

		ret = devm_clk_hw_register(chip->dev, &clk->hw);
		if (ret) {
			dev_err(chip->dev, "Failed to register clk '%s': %d\n",
				init.name, ret);
			goto err_unregister_rtc_32k;
		}

		chip->clk_data->hws[i] = &clk->hw;
	}

	chip->clk_data->num = i;
	ret = of_clk_add_hw_provider(np, of_clk_hw_onecell_get, chip->clk_data);
	if (ret)
		goto err_unregister_rtc_32k;

	return 0;

err_unregister_rtc_32k:
	clk_unregister_fixed_rate(chip->rtc_32k_clk->clk);

	return ret;
}

static void ac100_rtc_unregister_clks(struct ac100_rtc_dev *chip)
{
	of_clk_del_provider(chip->dev->of_node);
	clk_unregister_fixed_rate(chip->rtc_32k_clk->clk);
}

/*
/* …（同文件无关代码省略）… */
static int ac100_rtc_alarm_irq_enable(struct device *dev, unsigned int en)
{
	struct ac100_rtc_dev *chip = dev_get_drvdata(dev);
	struct regmap *regmap = chip->regmap;
	unsigned int val;

	val = en ? AC100_ALM_INT_ENABLE : 0;

	return regmap_write(regmap, AC100_ALM_INT_ENA, val);
}
/* …（同文件无关代码省略）… */
static irqreturn_t ac100_rtc_irq(int irq, void *data)
{
	struct ac100_rtc_dev *chip = data;
	struct regmap *regmap = chip->regmap;
	unsigned int val = 0;
	int ret;

	rtc_lock(chip->rtc);

	/* read status */
	ret = regmap_read(regmap, AC100_ALM_INT_STA, &val);
	if (ret)
		goto out;

	if (val & AC100_ALM_INT_ENABLE) {
		/* signal rtc framework */
		rtc_update_irq(chip->rtc, 1, RTC_AF | RTC_IRQF);

		/* clear status */
		ret = regmap_write(regmap, AC100_ALM_INT_STA, val);
		if (ret)
			goto out;

		/* disable interrupt */
		ret = ac100_rtc_alarm_irq_enable(chip->dev, 0);
		if (ret)
			goto out;
	}

out:
	rtc_unlock(chip->rtc);
	return IRQ_HANDLED;
}
/* …（同文件无关代码省略）… */
static int ac100_rtc_probe(struct platform_device *pdev)
{
	struct ac100_dev *ac100 = dev_get_drvdata(pdev->dev.parent);
	struct ac100_rtc_dev *chip;
	int ret;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	platform_set_drvdata(pdev, chip);
	chip->dev = &pdev->dev;
	chip->regmap = ac100->regmap;

	chip->irq = platform_get_irq(pdev, 0);
	if (chip->irq < 0)
		return chip->irq;

	chip->rtc = devm_rtc_allocate_device(&pdev->dev);
	if (IS_ERR(chip->rtc))
		return PTR_ERR(chip->rtc);

	chip->rtc->ops = &ac100_rtc_ops;

	ret = devm_request_threaded_irq(&pdev->dev, chip->irq, NULL,
					ac100_rtc_irq,
					IRQF_SHARED | IRQF_ONESHOT,
					dev_name(&pdev->dev), chip);
	if (ret) {
		dev_err(&pdev->dev, "Could not request IRQ\n");
		return ret;
	}

	/* always use 24 hour mode */
	regmap_write_bits(chip->regmap, AC100_RTC_CTRL, AC100_RTC_CTRL_24HOUR,
			  AC100_RTC_CTRL_24HOUR);

	/* disable counter alarm interrupt */
	regmap_write(chip->regmap, AC100_ALM_INT_ENA, 0);

	/* clear counter alarm pending interrupts */
	regmap_write(chip->regmap, AC100_ALM_INT_STA, AC100_ALM_INT_ENABLE);

	ret = ac100_rtc_register_clks(chip);
	if (ret)
		return ret;

	return devm_rtc_register_device(chip->rtc);
}

static void ac100_rtc_remove(struct platform_device *pdev)
{
	struct ac100_rtc_dev *chip = platform_get_drvdata(pdev);

	ac100_rtc_unregister_clks(chip);
}

static const struct of_device_id ac100_rtc_match[] = {
	{ .compatible = "x-powers,ac100-rtc" },
	{ },
/* …（同文件无关代码省略）… */

static struct platform_driver ac100_rtc_driver = {
	.probe		= ac100_rtc_probe,
	.remove		= ac100_rtc_remove,
	.driver		= {
		.name		= "ac100-rtc",
		.of_match_table	= of_match_ptr(ac100_rtc_match),
