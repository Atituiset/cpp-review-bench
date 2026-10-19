// AUTO-DRAFT from torvalds/linux PR #8486d357129126e27fb5d4e02622566bafc77035
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define IMX7ULP_CR	0x10
#define IMX7ULP_SR	0x14
#define IMX7ULP_IER	0x18
/* …（同文件无关代码省略）… */
#define IMX7ULP_FSR	0x5c
#define IMX7ULP_TCR	0x60
#define IMX7ULP_TDR	0x64
#define IMX7ULP_RSR	0x70
#define IMX7ULP_RDR	0x74
/* …（同文件无关代码省略）… */
#define CR_RRF		BIT(9)
#define CR_RTF		BIT(8)
/* …（同文件无关代码省略）… */
#define SR_MBF		BIT(24)
/* …（同文件无关代码省略）… */
#define SR_FCF		BIT(9)
/* …（同文件无关代码省略）… */
#define SR_TDF		BIT(0)
/* …（同文件无关代码省略）… */
#define IER_FCIE	BIT(9)
/* …（同文件无关代码省略）… */
#define IER_TDIE	BIT(0)
/* …（同文件无关代码省略）… */
#define FSR_TXCOUNT	(0xFF)
#define RSR_RXEMPTY	BIT(1)
/* …（同文件无关代码省略）… */
#define TCR_CONTC	BIT(20)
/* …（同文件无关代码省略）… */
#define SR_CLEAR_MASK	GENMASK(13, 8)
/* …（同文件无关代码省略）… */
struct fsl_lpspi_devtype_data {
	u8 prescale_max : 3; /* 0 == no limit */
	bool query_hw_for_num_cs : 1;
};
/* …（同文件无关代码省略）… */
struct lpspi_config {
	u8 bpw;
	u8 chip_select;
	u8 prescale;
	u32 mode;
	u32 speed_hz;
	u32 effective_speed_hz;
};
/* …（同文件无关代码省略）… */
struct fsl_lpspi_data {
	struct device *dev;
	void __iomem *base;
	unsigned long base_phys;
	struct clk *clk_ipg;
	struct clk *clk_per;
	bool is_target;
	bool is_only_cs1;
	bool is_first_byte;

	void *rx_buf;
	const void *tx_buf;
	void (*tx)(struct fsl_lpspi_data *fsl_lpspi);
	void (*rx)(struct fsl_lpspi_data *fsl_lpspi);

	u32 remain;
	u8 watermark;
	u8 txfifosize;
	u8 rxfifosize;

	struct lpspi_config config;
	struct completion xfer_done;

	bool target_aborted;

	/* DMA */
	bool usedma;
	struct completion dma_rx_completion;
	struct completion dma_tx_completion;

	const struct fsl_lpspi_devtype_data *devtype_data;
};
/* …（同文件无关代码省略）… */
static void fsl_lpspi_intctrl(struct fsl_lpspi_data *fsl_lpspi,
			      unsigned int enable)
{
	writel(enable, fsl_lpspi->base + IMX7ULP_IER);
}
/* …（同文件无关代码省略）… */
static int fsl_lpspi_bytes_per_word(const int bpw)
{
	return DIV_ROUND_UP(bpw, BITS_PER_BYTE);
}
/* …（同文件无关代码省略）… */
static void fsl_lpspi_write_tx_fifo(struct fsl_lpspi_data *fsl_lpspi)
{
	u8 txfifo_cnt;
	u32 temp;

	txfifo_cnt = readl(fsl_lpspi->base + IMX7ULP_FSR) & 0xff;

	while (txfifo_cnt < fsl_lpspi->txfifosize && fsl_lpspi->remain) {
		fsl_lpspi->tx(fsl_lpspi);
		txfifo_cnt++;
	}

	if (txfifo_cnt < fsl_lpspi->txfifosize) {
		if (!fsl_lpspi->is_target) {
			temp = readl(fsl_lpspi->base + IMX7ULP_TCR);
			temp &= ~TCR_CONTC;
			writel(temp, fsl_lpspi->base + IMX7ULP_TCR);
		}

		fsl_lpspi_intctrl(fsl_lpspi, IER_FCIE);
	} else {
		fsl_lpspi_intctrl(fsl_lpspi, IER_TDIE);
	}
}
/* …（同文件无关代码省略）… */
static void fsl_lpspi_read_rx_fifo(struct fsl_lpspi_data *fsl_lpspi)
{
	while (!(readl(fsl_lpspi->base + IMX7ULP_RSR) & RSR_RXEMPTY))
		fsl_lpspi->rx(fsl_lpspi);
}
/* …（同文件无关代码省略）… */
static int fsl_lpspi_dma_configure(struct spi_controller *controller)
{
	int ret;
	enum dma_slave_buswidth buswidth;
	struct dma_slave_config rx = {}, tx = {};
	struct fsl_lpspi_data *fsl_lpspi =
				spi_controller_get_devdata(controller);

	switch (fsl_lpspi_bytes_per_word(fsl_lpspi->config.bpw)) {
	case 4:
		buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
		break;
	case 2:
		buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
		break;
	case 1:
		buswidth = DMA_SLAVE_BUSWIDTH_1_BYTE;
		break;
	default:
		return -EINVAL;
	}

	tx.direction = DMA_MEM_TO_DEV;
	tx.dst_addr = fsl_lpspi->base_phys + IMX7ULP_TDR;
	tx.dst_addr_width = buswidth;
	tx.dst_maxburst = 1;
	ret = dmaengine_slave_config(controller->dma_tx, &tx);
	if (ret) {
		dev_err(fsl_lpspi->dev, "TX dma configuration failed with %d\n",
			ret);
		return ret;
	}

	rx.direction = DMA_DEV_TO_MEM;
	rx.src_addr = fsl_lpspi->base_phys + IMX7ULP_RDR;
	rx.src_addr_width = buswidth;
	rx.src_maxburst = 1;
	ret = dmaengine_slave_config(controller->dma_rx, &rx);
	if (ret) {
		dev_err(fsl_lpspi->dev, "RX dma configuration failed with %d\n",
			ret);
		return ret;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
	return 0;
}

static int fsl_lpspi_wait_for_completion(struct spi_controller *controller)
{
	struct fsl_lpspi_data *fsl_lpspi =
				spi_controller_get_devdata(controller);

	if (fsl_lpspi->is_target) {
		if (wait_for_completion_interruptible(&fsl_lpspi->xfer_done) ||
			fsl_lpspi->target_aborted) {
/* …（同文件无关代码省略）… */
static void fsl_lpspi_reset(struct fsl_lpspi_data *fsl_lpspi)
{
	u32 temp;

	if (!fsl_lpspi->usedma) {
		/* Disable all interrupt */
		fsl_lpspi_intctrl(fsl_lpspi, 0);
	}

	/* Clear FIFO and disable module */
	temp = CR_RRF | CR_RTF;
	writel(temp, fsl_lpspi->base + IMX7ULP_CR);

	/* W1C for all flags in SR */
	writel(SR_CLEAR_MASK, fsl_lpspi->base + IMX7ULP_SR);
}
/* …（同文件无关代码省略）… */
static void fsl_lpspi_dma_rx_callback(void *cookie)
{
	struct fsl_lpspi_data *fsl_lpspi = (struct fsl_lpspi_data *)cookie;

	complete(&fsl_lpspi->dma_rx_completion);
}
/* …（同文件无关代码省略）… */
static void fsl_lpspi_dma_tx_callback(void *cookie)
{
	struct fsl_lpspi_data *fsl_lpspi = (struct fsl_lpspi_data *)cookie;

	complete(&fsl_lpspi->dma_tx_completion);
}
/* …（同文件无关代码省略）… */
static int fsl_lpspi_calculate_timeout(struct fsl_lpspi_data *fsl_lpspi,
				       int size)
{
	unsigned long timeout = 0;

	/* Time with actual data transfer and CS change delay related to HW */
	timeout = (8 + 4) * size / fsl_lpspi->config.speed_hz;

	/* Add extra second for scheduler related activities */
	timeout += 1;

	/* Double calculated timeout */
	return secs_to_jiffies(2 * timeout);
}
/* …（同文件无关代码省略）… */
static int fsl_lpspi_dma_transfer(struct spi_controller *controller,
				struct fsl_lpspi_data *fsl_lpspi,
				struct spi_transfer *transfer)
{
	struct dma_async_tx_descriptor *desc_tx, *desc_rx;
	unsigned long transfer_timeout;
	unsigned long time_left;
	struct sg_table *tx = &transfer->tx_sg, *rx = &transfer->rx_sg;
	int ret;

	ret = fsl_lpspi_dma_configure(controller);
	if (ret)
		return ret;

	desc_rx = dmaengine_prep_slave_sg(controller->dma_rx,
				rx->sgl, rx->nents, DMA_DEV_TO_MEM,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!desc_rx)
		return -EINVAL;

	desc_rx->callback = fsl_lpspi_dma_rx_callback;
	desc_rx->callback_param = (void *)fsl_lpspi;
	dmaengine_submit(desc_rx);
	reinit_completion(&fsl_lpspi->dma_rx_completion);
	dma_async_issue_pending(controller->dma_rx);

	desc_tx = dmaengine_prep_slave_sg(controller->dma_tx,
				tx->sgl, tx->nents, DMA_MEM_TO_DEV,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!desc_tx) {
		dmaengine_terminate_sync(controller->dma_rx);
		return -EINVAL;
	}

	desc_tx->callback = fsl_lpspi_dma_tx_callback;
	desc_tx->callback_param = (void *)fsl_lpspi;
	dmaengine_submit(desc_tx);
	reinit_completion(&fsl_lpspi->dma_tx_completion);
	dma_async_issue_pending(controller->dma_tx);

	fsl_lpspi->target_aborted = false;

	if (!fsl_lpspi->is_target) {
		transfer_timeout = fsl_lpspi_calculate_timeout(fsl_lpspi,
							       transfer->len);

		/* Wait eDMA to finish the data transfer.*/
		time_left = wait_for_completion_timeout(&fsl_lpspi->dma_tx_completion,
							transfer_timeout);
		if (!time_left) {
			dev_err(fsl_lpspi->dev, "I/O Error in DMA TX\n");
			dmaengine_terminate_sync(controller->dma_tx);
			dmaengine_terminate_sync(controller->dma_rx);
			fsl_lpspi_reset(fsl_lpspi);
			return -ETIMEDOUT;
		}

		time_left = wait_for_completion_timeout(&fsl_lpspi->dma_rx_completion,
							transfer_timeout);
		if (!time_left) {
			dev_err(fsl_lpspi->dev, "I/O Error in DMA RX\n");
			dmaengine_terminate_sync(controller->dma_tx);
			dmaengine_terminate_sync(controller->dma_rx);
			fsl_lpspi_reset(fsl_lpspi);
			return -ETIMEDOUT;
		}
	} else {
		if (wait_for_completion_interruptible(&fsl_lpspi->dma_tx_completion) ||
			fsl_lpspi->target_aborted) {
			dev_dbg(fsl_lpspi->dev,
				"I/O Error in DMA TX interrupted\n");
			dmaengine_terminate_sync(controller->dma_tx);
			dmaengine_terminate_sync(controller->dma_rx);
			fsl_lpspi_reset(fsl_lpspi);
			return -EINTR;
		}

		if (wait_for_completion_interruptible(&fsl_lpspi->dma_rx_completion) ||
			fsl_lpspi->target_aborted) {
			dev_dbg(fsl_lpspi->dev,
				"I/O Error in DMA RX interrupted\n");
			dmaengine_terminate_sync(controller->dma_tx);
			dmaengine_terminate_sync(controller->dma_rx);
			fsl_lpspi_reset(fsl_lpspi);
			return -EINTR;
		}
	}

	fsl_lpspi_reset(fsl_lpspi);

	return 0;
}
/* …（同文件无关代码省略）… */
	return ret;
}

static int fsl_lpspi_pio_transfer(struct spi_controller *controller,
				  struct spi_transfer *t)
{
	struct fsl_lpspi_data *fsl_lpspi =
				spi_controller_get_devdata(controller);
	int ret;

	fsl_lpspi->tx_buf = t->tx_buf;
/* …（同文件无关代码省略）… */

	fsl_lpspi_write_tx_fifo(fsl_lpspi);

	ret = fsl_lpspi_wait_for_completion(controller);

	fsl_lpspi_reset(fsl_lpspi);

/* …（同文件无关代码省略）… */
	if (fsl_lpspi->usedma)
		ret = fsl_lpspi_dma_transfer(controller, fsl_lpspi, t);
	else
		ret = fsl_lpspi_pio_transfer(controller, t);
	if (ret < 0)
		return ret;

	return 0;
}

static irqreturn_t fsl_lpspi_isr(int irq, void *dev_id)
{
	u32 temp_SR, temp_IER;
	struct fsl_lpspi_data *fsl_lpspi = dev_id;

	temp_IER = readl(fsl_lpspi->base + IMX7ULP_IER);
	fsl_lpspi_intctrl(fsl_lpspi, 0);
	temp_SR = readl(fsl_lpspi->base + IMX7ULP_SR);

	fsl_lpspi_read_rx_fifo(fsl_lpspi);

	if ((temp_SR & SR_TDF) && (temp_IER & IER_TDIE)) {
		fsl_lpspi_write_tx_fifo(fsl_lpspi);
		return IRQ_HANDLED;
	}

	if (temp_SR & SR_MBF ||
	    readl(fsl_lpspi->base + IMX7ULP_FSR) & FSR_TXCOUNT) {
		writel(SR_FCF, fsl_lpspi->base + IMX7ULP_SR);
		fsl_lpspi_intctrl(fsl_lpspi, IER_FCIE | (temp_IER & IER_TDIE));
		return IRQ_HANDLED;
	}

	if (temp_SR & SR_FCF && (temp_IER & IER_FCIE)) {
		writel(SR_FCF, fsl_lpspi->base + IMX7ULP_SR);
		complete(&fsl_lpspi->xfer_done);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}
