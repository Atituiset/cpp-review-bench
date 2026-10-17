// AUTO-DRAFT from torvalds/linux PR #a3bb7a0184fbf9fed14aacb9773fe9f6f2097c1b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct acp6x_dev_data {
	void __iomem *acp6x_base;
	struct resource *res;
	bool acp6x_audio_mode;
	struct platform_device *pdev[ACP6x_DEVS];
};
/* …（同文件无关代码省略）… */
static int acp6x_reset(void __iomem *acp_base)
{
	u32 val;
	int timeout;

	acp6x_writel(1, acp_base + ACP_SOFT_RESET);
	timeout = 0;
	while (++timeout < 500) {
		val = acp6x_readl(acp_base + ACP_SOFT_RESET);
		if (val & ACP_SOFT_RESET_SOFTRESET_AUDDONE_MASK)
			break;
		cpu_relax();
	}
	acp6x_writel(0, acp_base + ACP_SOFT_RESET);
	timeout = 0;
	while (++timeout < 500) {
		val = acp6x_readl(acp_base + ACP_SOFT_RESET);
		if (!val)
			return 0;
		cpu_relax();
	}
	return -ETIMEDOUT;
}
/* …（同文件无关代码省略）… */
static void acp6x_disable_interrupts(void __iomem *acp_base)
{
	acp6x_writel(ACP_EXT_INTR_STAT_CLEAR_MASK, acp_base +
		     ACP_EXTERNAL_INTR_STAT);
	acp6x_writel(0x00, acp_base + ACP_EXTERNAL_INTR_CNTL);
	acp6x_writel(0x00, acp_base + ACP_EXTERNAL_INTR_ENB);
}
/* …（同文件无关代码省略）… */
static int acp6x_deinit(void __iomem *acp_base)
{
	int ret;

	acp6x_disable_interrupts(acp_base);
	/* Reset */
	ret = acp6x_reset(acp_base);
	if (ret) {
		pr_err("ACP reset failed\n");
		return ret;
	}
	acp6x_writel(0x00, acp_base + ACP_CLKMUX_SEL);
	acp6x_writel(0x00, acp_base + ACP_CONTROL);
	return 0;
}
/* …（同文件无关代码省略）… */
static int snd_acp6x_suspend(struct device *dev)
{
	struct acp6x_dev_data *adata;
	int ret;

	adata = dev_get_drvdata(dev);
	ret = acp6x_deinit(adata->acp6x_base);
	if (ret)
		dev_err(dev, "ACP de-init failed\n");
	return ret;
}

static int snd_acp6x_resume(struct device *dev)
{
	struct acp6x_dev_data *adata;
	int ret;
/* …（同文件无关代码省略）… */
	return ret;
}

static const struct dev_pm_ops acp6x_pm = {
	RUNTIME_PM_OPS(snd_acp6x_suspend, snd_acp6x_resume, NULL)
	SYSTEM_SLEEP_PM_OPS(snd_acp6x_suspend, snd_acp6x_resume)
};
