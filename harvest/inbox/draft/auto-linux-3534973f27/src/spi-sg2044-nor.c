// AUTO-DRAFT from torvalds/linux PR #f47959816751f02a711aab27cf1c805bd4964819
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define SPIFMC_TRAN_CSR				0x10
#define SPIFMC_TRAN_CSR_TRAN_MODE_MASK		GENMASK(1, 0)
#define SPIFMC_TRAN_CSR_TRAN_MODE_RX		BIT(0)
#define SPIFMC_TRAN_CSR_TRAN_MODE_TX		BIT(1)
#define SPIFMC_TRAN_CSR_FAST_MODE		BIT(3)
#define SPIFMC_TRAN_CSR_BUS_WIDTH_MASK		GENMASK(5, 4)
#define SPIFMC_TRAN_CSR_BUS_WIDTH_1_BIT		(0x00 << 4)
/* …（同文件无关代码省略）… */
#define SPIFMC_TRAN_CSR_DMA_EN			BIT(6)
/* …（同文件无关代码省略）… */
#define SPIFMC_TRAN_CSR_ADDR_BYTES_MASK		GENMASK(10, 8)
#define SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT	8
#define SPIFMC_TRAN_CSR_WITH_CMD		BIT(11)
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_MASK	GENMASK(13, 12)
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE	(0x00 << 12)
/* …（同文件无关代码省略）… */
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_8_BYTE	(0x03 << 12)
#define SPIFMC_TRAN_CSR_GO_BUSY			BIT(15)
/* …（同文件无关代码省略）… */
#define SPIFMC_TRAN_NUM				0x14
#define SPIFMC_FIFO_PORT			0x18
#define SPIFMC_FIFO_PT				0x20
/* …（同文件无关代码省略）… */
#define SPIFMC_INT_STS				0x28
#define SPIFMC_INT_TRAN_DONE			BIT(0)
#define SPIFMC_INT_RD_FIFO			BIT(2)
/* …（同文件无关代码省略）… */
#define SPIFMC_OPT				0x030
#define SPIFMC_OPT_DISABLE_FIFO_FLUSH		BIT(1)
/* …（同文件无关代码省略）… */
#define SPIFMC_MAX_FIFO_DEPTH			8
/* …（同文件无关代码省略）… */
#define SPIFMC_MAX_READ_SIZE			0x10000
/* …（同文件无关代码省略）… */
struct sg204x_spifmc_chip_info {
	bool has_opt_reg;
	u32 rd_fifo_int_trigger_level;
};
/* …（同文件无关代码省略）… */
struct sg2044_spifmc {
	struct spi_controller *ctrl;
	void __iomem *io_base;
	struct device *dev;
	struct mutex lock;
	struct clk *clk;
	const struct sg204x_spifmc_chip_info *chip_info;
};
/* …（同文件无关代码省略）… */
static int sg2044_spifmc_wait_int(struct sg2044_spifmc *spifmc, u8 int_type)
{
	u32 stat;

	return readl_poll_timeout(spifmc->io_base + SPIFMC_INT_STS, stat,
				  (stat & int_type), 0, 1000000);
}
/* …（同文件无关代码省略）… */
static int sg2044_spifmc_wait_xfer_size(struct sg2044_spifmc *spifmc,
					int xfer_size)
{
	u8 stat;

	return readl_poll_timeout(spifmc->io_base + SPIFMC_FIFO_PT, stat,
				  ((stat & 0xf) == xfer_size), 1, 1000000);
}
/* …（同文件无关代码省略）… */
static u32 sg2044_spifmc_init_reg(struct sg2044_spifmc *spifmc)
{
	u32 reg;

	reg = readl(spifmc->io_base + SPIFMC_TRAN_CSR);
	reg &= ~(SPIFMC_TRAN_CSR_TRAN_MODE_MASK |
		 SPIFMC_TRAN_CSR_FAST_MODE |
		 SPIFMC_TRAN_CSR_BUS_WIDTH_MASK |
		 SPIFMC_TRAN_CSR_DMA_EN |
		 SPIFMC_TRAN_CSR_ADDR_BYTES_MASK |
		 SPIFMC_TRAN_CSR_WITH_CMD |
		 SPIFMC_TRAN_CSR_FIFO_TRG_LVL_MASK);

	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);

	return reg;
}
/* …（同文件无关代码省略）… */
static ssize_t sg2044_spifmc_read_64k(struct sg2044_spifmc *spifmc,
				      const struct spi_mem_op *op, loff_t from,
				      size_t len, u_char *buf)
{
	int xfer_size, offset;
	u32 reg;
	int ret;
	int i;

	reg = sg2044_spifmc_init_reg(spifmc);
	reg |= (op->addr.nbytes + op->dummy.nbytes) << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
	reg |= spifmc->chip_info->rd_fifo_int_trigger_level;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;
	reg |= SPIFMC_TRAN_CSR_TRAN_MODE_RX;

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);
	writeb(op->cmd.opcode, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = op->addr.nbytes - 1; i >= 0; i--)
		writeb((from >> i * 8) & 0xff, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = 0; i < op->dummy.nbytes; i++)
		writeb(0xff, spifmc->io_base + SPIFMC_FIFO_PORT);

	writel(len, spifmc->io_base + SPIFMC_TRAN_NUM);
	writel(0, spifmc->io_base + SPIFMC_INT_STS);
	reg |= SPIFMC_TRAN_CSR_GO_BUSY;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);

	ret = sg2044_spifmc_wait_int(spifmc, SPIFMC_INT_RD_FIFO);
	if (ret < 0)
		return ret;

	offset = 0;
	while (offset < len) {
		xfer_size = min_t(size_t, SPIFMC_MAX_FIFO_DEPTH, len - offset);

		ret = sg2044_spifmc_wait_xfer_size(spifmc, xfer_size);
		if (ret < 0)
			return ret;

		for (i = 0; i < xfer_size; i++)
			buf[i + offset] = readb(spifmc->io_base + SPIFMC_FIFO_PORT);

		offset += xfer_size;
	}

	ret = sg2044_spifmc_wait_int(spifmc, SPIFMC_INT_TRAN_DONE);
	if (ret < 0)
		return ret;

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	return len;
}
/* …（同文件无关代码省略）… */
static ssize_t sg2044_spifmc_read(struct sg2044_spifmc *spifmc,
				  const struct spi_mem_op *op)
{
	size_t xfer_size;
	size_t offset;
	loff_t from = op->addr.val;
	size_t len = op->data.nbytes;
	int ret;
	u8 *din = op->data.buf.in;

	offset = 0;
	while (offset < len) {
		xfer_size = min_t(size_t, SPIFMC_MAX_READ_SIZE, len - offset);

		ret = sg2044_spifmc_read_64k(spifmc, op, from, xfer_size, din);
		if (ret < 0)
			return ret;

		offset += xfer_size;
		din += xfer_size;
		from += xfer_size;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static ssize_t sg2044_spifmc_write(struct sg2044_spifmc *spifmc,
				   const struct spi_mem_op *op)
{
	size_t xfer_size;
	const u8 *dout = op->data.buf.out;
	int i, offset;
	int ret;
	u32 reg;

	reg = sg2044_spifmc_init_reg(spifmc);
	reg |= (op->addr.nbytes + op->dummy.nbytes) << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
	reg |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_8_BYTE;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;
	reg |= SPIFMC_TRAN_CSR_TRAN_MODE_TX;

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);
	writeb(op->cmd.opcode, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = op->addr.nbytes - 1; i >= 0; i--)
		writeb((op->addr.val >> i * 8) & 0xff, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = 0; i < op->dummy.nbytes; i++)
		writeb(0xff, spifmc->io_base + SPIFMC_FIFO_PORT);

	writel(0, spifmc->io_base + SPIFMC_INT_STS);
	writel(op->data.nbytes, spifmc->io_base + SPIFMC_TRAN_NUM);
	reg |= SPIFMC_TRAN_CSR_GO_BUSY;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);

	ret = sg2044_spifmc_wait_xfer_size(spifmc, 0);
	if (ret < 0)
		return ret;

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	offset = 0;
	while (offset < op->data.nbytes) {
		xfer_size = min_t(size_t, SPIFMC_MAX_FIFO_DEPTH, op->data.nbytes - offset);

		ret = sg2044_spifmc_wait_xfer_size(spifmc, 0);
		if (ret < 0)
			return ret;

		for (i = 0; i < xfer_size; i++)
			writeb(dout[i + offset], spifmc->io_base + SPIFMC_FIFO_PORT);

		offset += xfer_size;
	}

	ret = sg2044_spifmc_wait_int(spifmc, SPIFMC_INT_TRAN_DONE);
	if (ret < 0)
		return ret;

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	return 0;
}
/* …（同文件无关代码省略）… */
static ssize_t sg2044_spifmc_tran_cmd(struct sg2044_spifmc *spifmc,
				      const struct spi_mem_op *op)
{
	int i, ret;
	u32 reg;

	reg = sg2044_spifmc_init_reg(spifmc);
	reg |= (op->addr.nbytes + op->dummy.nbytes) << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
	reg |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);
	writeb(op->cmd.opcode, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = op->addr.nbytes - 1; i >= 0; i--)
		writeb((op->addr.val >> i * 8) & 0xff, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = 0; i < op->dummy.nbytes; i++)
		writeb(0xff, spifmc->io_base + SPIFMC_FIFO_PORT);

	writel(0, spifmc->io_base + SPIFMC_INT_STS);
	reg |= SPIFMC_TRAN_CSR_GO_BUSY;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);

	ret = sg2044_spifmc_wait_int(spifmc, SPIFMC_INT_TRAN_DONE);
	if (ret < 0)
		return ret;

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	return 0;
}

static void sg2044_spifmc_trans(struct sg2044_spifmc *spifmc,
				const struct spi_mem_op *op)
{
	if (op->data.dir == SPI_MEM_DATA_IN)
		sg2044_spifmc_read(spifmc, op);
	else if (op->data.dir == SPI_MEM_DATA_OUT)
		sg2044_spifmc_write(spifmc, op);
	else
		sg2044_spifmc_tran_cmd(spifmc, op);
}

static ssize_t sg2044_spifmc_trans_reg(struct sg2044_spifmc *spifmc,
				       const struct spi_mem_op *op)
{
	const u8 *dout = NULL;
	u8 *din = NULL;
	size_t len = op->data.nbytes;
	int ret, i;
	u32 reg;

	if (op->data.dir == SPI_MEM_DATA_IN)
		din = op->data.buf.in;
	else
		dout = op->data.buf.out;

	reg = sg2044_spifmc_init_reg(spifmc);
	reg |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;

	if (din) {
		reg |= SPIFMC_TRAN_CSR_BUS_WIDTH_1_BIT;
		reg |= SPIFMC_TRAN_CSR_TRAN_MODE_RX;
		reg |= SPIFMC_TRAN_CSR_TRAN_MODE_TX;

		if (spifmc->chip_info->has_opt_reg)
			writel(SPIFMC_OPT_DISABLE_FIFO_FLUSH, spifmc->io_base + SPIFMC_OPT);
	} else {
		/*
		 * If write values to the Status Register,
		 * configure TRAN_CSR register as the same as
		 * sg2044_spifmc_read_reg.
		 */
		if (op->cmd.opcode == 0x01) {
			reg |= SPIFMC_TRAN_CSR_TRAN_MODE_RX;
			reg |= SPIFMC_TRAN_CSR_TRAN_MODE_TX;
			writel(len, spifmc->io_base + SPIFMC_TRAN_NUM);
		}
	}

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);
	writeb(op->cmd.opcode, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = 0; i < len; i++) {
		if (din)
			writeb(0xff, spifmc->io_base + SPIFMC_FIFO_PORT);
		else
			writeb(dout[i], spifmc->io_base + SPIFMC_FIFO_PORT);
	}

	writel(0, spifmc->io_base + SPIFMC_INT_STS);
	writel(len, spifmc->io_base + SPIFMC_TRAN_NUM);
	reg |= SPIFMC_TRAN_CSR_GO_BUSY;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);

	ret = sg2044_spifmc_wait_int(spifmc, SPIFMC_INT_TRAN_DONE);
	if (ret < 0)
		return ret;

	if (din) {
		while (len--)
			*din++ = readb(spifmc->io_base + SPIFMC_FIFO_PORT);
	}

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	return 0;
}

static int sg2044_spifmc_exec_op(struct spi_mem *mem,
				 const struct spi_mem_op *op)
{
	struct sg2044_spifmc *spifmc;

	spifmc = spi_controller_get_devdata(mem->spi->controller);

	mutex_lock(&spifmc->lock);

	if (op->addr.nbytes == 0)
		sg2044_spifmc_trans_reg(spifmc, op);
	else
		sg2044_spifmc_trans(spifmc, op);

	mutex_unlock(&spifmc->lock);

	return 0;
}

static const struct spi_controller_mem_ops sg2044_spifmc_mem_ops = {
	.exec_op = sg2044_spifmc_exec_op,
};

static void sg2044_spifmc_init(struct sg2044_spifmc *spifmc)
{
	u32 tran_csr;
/* …（同文件无关代码省略）… */
	if (IS_ERR(spifmc->clk))
		return dev_err_probe(dev, PTR_ERR(spifmc->clk), "Cannot get and enable AHB clock\n");

	spifmc->dev = &pdev->dev;
	spifmc->ctrl = ctrl;

/* …（同文件无关代码省略）… */
	ctrl->bits_per_word_mask = SPI_BPW_MASK(8);
	ctrl->auto_runtime_pm = false;
	ctrl->mem_ops = &sg2044_spifmc_mem_ops;
	ctrl->mode_bits = SPI_RX_DUAL | SPI_TX_DUAL | SPI_RX_QUAD | SPI_TX_QUAD;

	ret = devm_mutex_init(dev, &spifmc->lock);
