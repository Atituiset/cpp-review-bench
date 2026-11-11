// AUTO-DRAFT from torvalds/linux PR #f7c68e9d5d050301548d93ccbccd644151ced269
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdint.h>
  // <<< BUG ANCHOR
#define JZ_REG_MMC_STRPCL	0x00
#define JZ_REG_MMC_STATUS	0x04
/* …（同文件无关代码省略）… */
#define JZ_REG_MMC_IMASK	0x24
/* …（同文件无关代码省略）… */
#define JZ_MMC_STRPCL_RESET BIT(3)
/* …（同文件无关代码省略）… */
#define JZ_MMC_STATUS_IS_RESETTING BIT(15)
/* …（同文件无关代码省略）… */
enum jz4740_mmc_version {
	JZ_MMC_JZ4740,
	JZ_MMC_JZ4725B,
	JZ_MMC_JZ4760,
	JZ_MMC_JZ4780,
	JZ_MMC_X1000,
};
/* …（同文件无关代码省略）… */
enum jz4740_mmc_state {
	JZ4740_MMC_STATE_READ_RESPONSE,
	JZ4740_MMC_STATE_TRANSFER_DATA,
	JZ4740_MMC_STATE_SEND_STOP,
	JZ4740_MMC_STATE_DONE,
};
/* …（同文件无关代码省略）… */
struct jz4740_mmc_host {
	struct mmc_host *mmc;
	struct platform_device *pdev;
	struct clk *clk;

	enum jz4740_mmc_version version;

	int irq;

	void __iomem *base;
	struct resource *mem_res;
	struct mmc_request *req;
	struct mmc_command *cmd;

	bool vqmmc_enabled;

	unsigned long waiting;

	uint32_t cmdat;

	uint32_t irq_mask;

	spinlock_t lock;

	struct timer_list timeout_timer;
	struct sg_mapping_iter miter;
	enum jz4740_mmc_state state;

	/* DMA support */
	struct dma_chan *dma_rx;
	struct dma_chan *dma_tx;
	bool use_dma;

/* The DMA trigger level is 8 words, that is to say, the DMA read
 * trigger is when data words in MSC_RXFIFO is >= 8 and the DMA write
 * trigger is when data words in MSC_TXFIFO is < 8.
 */
#define JZ4740_MMC_FIFO_HALF_SIZE 8
};
/* …（同文件无关代码省略）… */
static void jz4740_mmc_write_irq_mask(struct jz4740_mmc_host *host,
				      uint32_t val)
{
	if (host->version >= JZ_MMC_JZ4725B)
		return writel(val, host->base + JZ_REG_MMC_IMASK);
	else
		return writew(val, host->base + JZ_REG_MMC_IMASK);
}
/* …（同文件无关代码省略）… */
static void jz4740_mmc_set_irq_enabled(struct jz4740_mmc_host *host,
	unsigned int irq, bool enabled)
{
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	if (enabled)
		host->irq_mask &= ~irq;
	else
		host->irq_mask |= irq;

	jz4740_mmc_write_irq_mask(host, host->irq_mask);
	spin_unlock_irqrestore(&host->lock, flags);
}
/* …（同文件无关代码省略）… */
static void jz4740_mmc_reset(struct jz4740_mmc_host *host)
{
	uint32_t status;
	unsigned int timeout = 1000;

	writew(JZ_MMC_STRPCL_RESET, host->base + JZ_REG_MMC_STRPCL);
	udelay(10);
	do {
		status = readl(host->base + JZ_REG_MMC_STATUS);
	} while (status & JZ_MMC_STATUS_IS_RESETTING && --timeout);
}
/* …（同文件无关代码省略）… */
{
	struct jz4740_mmc_host *host = platform_get_drvdata(pdev);

	timer_delete_sync(&host->timeout_timer);
	jz4740_mmc_set_irq_enabled(host, 0xff, false);
	jz4740_mmc_reset(host);

	mmc_remove_host(host->mmc);

	free_irq(host->irq, host);

	if (host->use_dma)
