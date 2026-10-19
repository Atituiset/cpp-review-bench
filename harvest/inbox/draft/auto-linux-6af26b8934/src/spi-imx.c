// AUTO-DRAFT from torvalds/linux PR #8486d357129126e27fb5d4e02622566bafc77035
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
#define BYTES_PER_32BITS_WORD		4
/* …（同文件无关代码省略）… */
enum spi_imx_devtype {
	IMX1_CSPI,
	IMX21_CSPI,
	IMX27_CSPI,
	IMX31_CSPI,
	IMX35_CSPI,	/* CSPI on all i.mx except above */
	IMX51_ECSPI,	/* ECSPI on i.mx51 */
	IMX53_ECSPI,	/* ECSPI on i.mx53 and later */
};
/* …（同文件无关代码省略）… */
struct spi_imx_devtype_data {
	void (*intctrl)(struct spi_imx_data *spi_imx, int enable);
	int (*prepare_message)(struct spi_imx_data *spi_imx, struct spi_message *msg);
	int (*prepare_transfer)(struct spi_imx_data *spi_imx, struct spi_device *spi,
				struct spi_transfer *t);
	void (*trigger)(struct spi_imx_data *spi_imx);
	int (*rx_available)(struct spi_imx_data *spi_imx);
	void (*reset)(struct spi_imx_data *spi_imx);
	void (*setup_wml)(struct spi_imx_data *spi_imx);
	void (*disable)(struct spi_imx_data *spi_imx);
	bool has_dmamode;
	bool has_targetmode;
	unsigned int fifo_size;
	bool dynamic_burst;
	/*
	 * ERR009165 fixed or not:
	 * https://www.nxp.com/docs/en/errata/IMX6DQCE.pdf
	 */
	bool tx_glitch_fixed;
	enum spi_imx_devtype devtype;
};
/* …（同文件无关代码省略）… */
struct dma_data_package {
	u32 cmd_word;
	void *dma_rx_buf;
	void *dma_tx_buf;
	dma_addr_t dma_tx_addr;
	dma_addr_t dma_rx_addr;
	int dma_len;
	int data_len;
};
/* …（同文件无关代码省略）… */
struct spi_imx_data {
	struct spi_controller *controller;
	struct device *dev;

	struct completion xfer_done;
	void __iomem *base;
	unsigned long base_phys;

	struct clk *clk_per;
	struct clk *clk_ipg;
	unsigned long spi_clk;
	unsigned int spi_bus_clk;

	unsigned int bits_per_word;
	unsigned int spi_drctl;

	unsigned int count, remainder;
	void (*tx)(struct spi_imx_data *spi_imx);
	void (*rx)(struct spi_imx_data *spi_imx);
	void *rx_buf;
	const void *tx_buf;
	unsigned int txfifo; /* number of words pushed in tx FIFO */
	unsigned int dynamic_burst;
	bool rx_only;

	/* Target mode */
	bool target_mode;
	bool target_aborted;
	unsigned int target_burst;

	/* DMA */
	bool usedma;
	u32 wml;
	struct completion dma_rx_completion;
	struct completion dma_tx_completion;
	size_t dma_package_num;
	struct dma_data_package *dma_data;
	int rx_offset;

	const struct spi_imx_devtype_data *devtype_data;
};
/* …（同文件无关代码省略）… */
static int spi_imx_bytes_per_word(const int bits_per_word)
{
	if (bits_per_word <= 8)
		return 1;
	else if (bits_per_word <= 16)
		return 2;
	else
		return 4;
}
/* …（同文件无关代码省略）… */
static void spi_imx_dma_rx_data_handle(struct spi_imx_data *spi_imx,
				       struct dma_data_package *dma_data, void *rx_buf,
				       bool word_delay)
{
	void *copy_ptr;
	int unaligned;

	/*
	 * On little-endian CPUs, adjust byte order:
	 * - Swap bytes when bpw = 8
	 * - Swap half-words when bpw = 16
	 * This ensures correct data ordering for DMA transfers.
	 */
#ifdef __LITTLE_ENDIAN
	if (!word_delay) {
		unsigned int bytes_per_word = spi_imx_bytes_per_word(spi_imx->bits_per_word);
		u32 *temp = dma_data->dma_rx_buf;

		for (int i = 0; i < DIV_ROUND_UP(dma_data->dma_len, sizeof(*temp)); i++) {
			if (bytes_per_word == 1)
				swab32s(temp + i);
			else if (bytes_per_word == 2)
				swahw32s(temp + i);
		}
	}
#endif

	/*
	 * When dynamic burst enabled, DMA RX always receives 32-bit words from RXFIFO with
	 * buswidth = 4, but when data_len is not 4-bytes alignment, the RM shows when
	 * burst length = 32*n + m bits, a SPI burst contains the m LSB in first word and all
	 * 32 bits in other n words. So if garbage bytes in the first word, trim first word then
	 * copy the actual data to rx_buf.
	 */
	if (dma_data->data_len % BYTES_PER_32BITS_WORD && !word_delay) {
		unaligned = dma_data->data_len % BYTES_PER_32BITS_WORD;
		copy_ptr = (u8 *)dma_data->dma_rx_buf + BYTES_PER_32BITS_WORD - unaligned;
	} else {
		copy_ptr = dma_data->dma_rx_buf;
	}

	memcpy(rx_buf, copy_ptr, dma_data->data_len);
}
/* …（同文件无关代码省略）… */
		dma_data->dma_len = ALIGN(dma_data->data_len, BYTES_PER_32BITS_WORD);
	}

	dma_data->dma_tx_buf = kzalloc(dma_data->dma_len, GFP_KERNEL);
	if (!dma_data->dma_tx_buf)
		return -ENOMEM;

	dma_data->dma_rx_buf = kzalloc(dma_data->dma_len, GFP_KERNEL);
	if (!dma_data->dma_rx_buf) {
		kfree(dma_data->dma_tx_buf);
		return -ENOMEM;
/* …（同文件无关代码省略）… */

	/* Trim the DMA RX buffer and copy the actual data to rx_buf */
	dma_sync_single_for_cpu(controller->dma_rx->device->dev, dma_data->dma_rx_addr,
				dma_data->dma_len, DMA_FROM_DEVICE);
	spi_imx_dma_rx_data_handle(spi_imx, dma_data, transfer->rx_buf + spi_imx->rx_offset,
				   word_delay);
	spi_imx->rx_offset += dma_data->data_len;
