// AUTO-DRAFT from torvalds/linux PR #b62f58bd7187ddbefa66913ec742137680ca5b22
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define SE_I2C_TX_TRANS_LEN		0x26c
#define SE_I2C_RX_TRANS_LEN		0x270
/* …（同文件无关代码省略）… */
#define I2C_WRITE		0x1
#define I2C_READ		0x2
/* …（同文件无关代码省略）… */
#define STOP_STRETCH		BIT(2)
/* …（同文件无关代码省略）… */
#define SLV_ADDR_MSK		GENMASK(15, 9)
#define SLV_ADDR_SHFT		9
/* …（同文件无关代码省略）… */
#define I2C_PACK_TX		BIT(0)
#define I2C_PACK_RX		BIT(1)
/* …（同文件无关代码省略）… */
#define ABORT_TIMEOUT		HZ
#define CANCEL_TIMEOUT		HZ
#define XFER_TIMEOUT		HZ
#define RST_TIMEOUT		HZ
/* …（同文件无关代码省略）… */
struct geni_i2c_desc {
	bool no_dma_support;
	unsigned int tx_fifo_depth;
	int (*resources_init)(struct geni_se *se);
	int (*set_rate)(struct geni_se *se, unsigned long freq);
	int (*power_on)(struct geni_se *se);
	int (*power_off)(struct geni_se *se);
};
/* …（同文件无关代码省略）… */
#define QCOM_I2C_MIN_NUM_OF_MSGS_MULTI_DESC	2
/* …（同文件无关代码省略）… */
struct geni_i2c_dev {
	struct geni_se se;
	u32 tx_wm;
	int irq;
	int err;
	struct i2c_adapter adap;
	struct completion done;
	struct completion abort_done;
	struct completion cancel_done;
	struct completion tx_reset_done;
	struct completion rx_reset_done;
	struct i2c_msg *cur;
	int cur_wr;
	int cur_rd;
	spinlock_t lock;
	u32 clk_freq_out;
	const struct geni_i2c_clk_fld *clk_fld;
	u32 clk_idx;
	void *dma_buf;
	size_t xfer_len;
	dma_addr_t dma_addr;
	struct dma_chan *tx_c;
	struct dma_chan *rx_c;
	bool no_dma;
	bool gpi_mode;
	bool is_tx_multi_desc_xfer;
	u32 num_msgs;
	struct geni_i2c_gpi_multi_desc_xfer i2c_multi_desc_config;
	const struct geni_i2c_desc *dev_data;
};
/* …（同文件无关代码省略）… */
struct geni_i2c_clk_fld {
	u32	clk_freq_out;
	u8	clk_div;
	u8	t_high_cnt;
	u8	t_low_cnt;
	u8	t_cycle_cnt;
};
/* …（同文件无关代码省略）… */
static void geni_i2c_err_misc(struct geni_i2c_dev *gi2c)
{
	u32 m_cmd = readl_relaxed(gi2c->se.base + SE_GENI_M_CMD0);
	u32 m_stat = readl_relaxed(gi2c->se.base + SE_GENI_M_IRQ_STATUS);
	u32 geni_s = readl_relaxed(gi2c->se.base + SE_GENI_STATUS);
	u32 geni_ios = readl_relaxed(gi2c->se.base + SE_GENI_IOS);
	u32 dma = readl_relaxed(gi2c->se.base + SE_GENI_DMA_MODE_EN);
	u32 rx_st, tx_st;

	if (dma) {
		rx_st = readl_relaxed(gi2c->se.base + SE_DMA_RX_IRQ_STAT);
		tx_st = readl_relaxed(gi2c->se.base + SE_DMA_TX_IRQ_STAT);
	} else {
		rx_st = readl_relaxed(gi2c->se.base + SE_GENI_RX_FIFO_STATUS);
		tx_st = readl_relaxed(gi2c->se.base + SE_GENI_TX_FIFO_STATUS);
	}
	dev_dbg(gi2c->se.dev, "DMA:%d tx_stat:0x%x, rx_stat:0x%x, irq-stat:0x%x\n",
		dma, tx_st, rx_st, m_stat);
	dev_dbg(gi2c->se.dev, "m_cmd:0x%x, geni_status:0x%x, geni_ios:0x%x\n",
		m_cmd, geni_s, geni_ios);
}
/* …（同文件无关代码省略）… */
static void geni_i2c_err(struct geni_i2c_dev *gi2c, int err)
{
	if (!gi2c->err)
		gi2c->err = gi2c_log[err].err;
	if (gi2c->cur)
		dev_dbg(gi2c->se.dev, "len:%d, slv-addr:0x%x, RD/WR:%d\n",
			gi2c->cur->len, gi2c->cur->addr, gi2c->cur->flags);

	trace_geni_i2c_err(gi2c->se.dev, gi2c_log[err].err, gi2c_log[err].msg);

	switch (err) {
	case GENI_ABORT_DONE:
	case ADDR_NACK:
	case DATA_NACK:
	case GENI_TIMEOUT:
		dev_dbg(gi2c->se.dev, "%s\n", gi2c_log[err].msg);
		break;
	default:
		dev_err(gi2c->se.dev, "%s\n", gi2c_log[err].msg);
		geni_i2c_err_misc(gi2c);
		break;
	}
}
/* …（同文件无关代码省略）… */
static void geni_i2c_abort_xfer(struct geni_i2c_dev *gi2c)
{
	unsigned long time_left = ABORT_TIMEOUT;
	unsigned long flags;

	reinit_completion(&gi2c->abort_done);

	spin_lock_irqsave(&gi2c->lock, flags);
	geni_se_abort_m_cmd(&gi2c->se);
	spin_unlock_irqrestore(&gi2c->lock, flags);

	time_left = wait_for_completion_timeout(&gi2c->abort_done, time_left);
	if (!time_left)
		dev_err(gi2c->se.dev, "Timeout abort_m_cmd\n");
}
/* …（同文件无关代码省略）… */
static void geni_i2c_cancel_xfer(struct geni_i2c_dev *gi2c)
{
	unsigned long time_left = msecs_to_jiffies(CANCEL_TIMEOUT);
	unsigned long flags;

	reinit_completion(&gi2c->cancel_done);

	spin_lock_irqsave(&gi2c->lock, flags);
	if (!gi2c->err)
		geni_i2c_err(gi2c, GENI_TIMEOUT);
	gi2c->cur = NULL;
	geni_se_cancel_m_cmd(&gi2c->se);
	spin_unlock_irqrestore(&gi2c->lock, flags);

	time_left = wait_for_completion_timeout(&gi2c->cancel_done, time_left);
	if (!time_left) {
		dev_err(gi2c->se.dev, "Timeout cancel_m_cmd\n");
		geni_i2c_abort_xfer(gi2c);
	}
}
/* …（同文件无关代码省略）… */
static void geni_i2c_rx_fsm_rst(struct geni_i2c_dev *gi2c)
{
	unsigned long time_left = RST_TIMEOUT;

	reinit_completion(&gi2c->rx_reset_done);
	writel_relaxed(1, gi2c->se.base + SE_DMA_RX_FSM_RST);

	time_left = wait_for_completion_timeout(&gi2c->rx_reset_done, time_left);
	if (!time_left)
		dev_err(gi2c->se.dev, "Timeout resetting RX_FSM\n");
}
/* …（同文件无关代码省略）… */
static void geni_i2c_tx_fsm_rst(struct geni_i2c_dev *gi2c)
{
	unsigned long time_left = RST_TIMEOUT;

	reinit_completion(&gi2c->tx_reset_done);
	writel_relaxed(1, gi2c->se.base + SE_DMA_TX_FSM_RST);

	time_left = wait_for_completion_timeout(&gi2c->tx_reset_done, time_left);
	if (!time_left)
		dev_err(gi2c->se.dev, "Timeout resetting TX_FSM\n");
}
/* …（同文件无关代码省略）… */
static void geni_i2c_rx_msg_cleanup(struct geni_i2c_dev *gi2c,
				     struct i2c_msg *cur)
{
	gi2c->cur_rd = 0;
	if (gi2c->dma_buf) {
		if (gi2c->err && gi2c->err != gi2c_log[ADDR_NACK].err)
			geni_i2c_rx_fsm_rst(gi2c);
		geni_se_rx_dma_unprep(&gi2c->se, gi2c->dma_addr, gi2c->xfer_len);
		i2c_put_dma_safe_msg_buf(gi2c->dma_buf, cur, !gi2c->err);
	}
}
/* …（同文件无关代码省略）… */
static void geni_i2c_tx_msg_cleanup(struct geni_i2c_dev *gi2c,
				     struct i2c_msg *cur)
{
	gi2c->cur_wr = 0;
	if (gi2c->dma_buf) {
		if (gi2c->err && gi2c->err != gi2c_log[ADDR_NACK].err)
			geni_i2c_tx_fsm_rst(gi2c);
		geni_se_tx_dma_unprep(&gi2c->se, gi2c->dma_addr, gi2c->xfer_len);
		i2c_put_dma_safe_msg_buf(gi2c->dma_buf, cur, !gi2c->err);
	}
}
/* …（同文件无关代码省略）… */
static int geni_i2c_rx_one_msg(struct geni_i2c_dev *gi2c, struct i2c_msg *msg,
				u32 m_param)
{
	dma_addr_t rx_dma = 0;
	unsigned long time_left;
	void *dma_buf;
	struct geni_se *se = &gi2c->se;
	size_t len = msg->len;
	struct i2c_msg *cur;

	dma_buf = gi2c->no_dma ? NULL : i2c_get_dma_safe_msg_buf(msg, 32);
	if (dma_buf)
		geni_se_select_mode(se, GENI_SE_DMA);
	else
		geni_se_select_mode(se, GENI_SE_FIFO);

	writel_relaxed(len, se->base + SE_I2C_RX_TRANS_LEN);
	geni_se_setup_m_cmd(se, I2C_READ, m_param);

	if (dma_buf && geni_se_rx_dma_prep(se, dma_buf, len, &rx_dma)) {
		geni_se_select_mode(se, GENI_SE_FIFO);
		i2c_put_dma_safe_msg_buf(dma_buf, msg, false);
		dma_buf = NULL;
	} else {
		gi2c->xfer_len = len;
		gi2c->dma_addr = rx_dma;
		gi2c->dma_buf = dma_buf;
	}

	cur = gi2c->cur;
	time_left = wait_for_completion_timeout(&gi2c->done, XFER_TIMEOUT);
	if (!time_left || (gi2c->err && gi2c->err != gi2c_log[ADDR_NACK].err))
		geni_i2c_cancel_xfer(gi2c);

	geni_i2c_rx_msg_cleanup(gi2c, cur);

	return gi2c->err;
}
/* …（同文件无关代码省略）… */
static int geni_i2c_tx_one_msg(struct geni_i2c_dev *gi2c, struct i2c_msg *msg,
				u32 m_param)
{
	dma_addr_t tx_dma = 0;
	unsigned long time_left;
	void *dma_buf;
	struct geni_se *se = &gi2c->se;
	size_t len = msg->len;
	struct i2c_msg *cur;

	dma_buf = gi2c->no_dma ? NULL : i2c_get_dma_safe_msg_buf(msg, 32);
	if (dma_buf)
		geni_se_select_mode(se, GENI_SE_DMA);
	else
		geni_se_select_mode(se, GENI_SE_FIFO);

	writel_relaxed(len, se->base + SE_I2C_TX_TRANS_LEN);
	geni_se_setup_m_cmd(se, I2C_WRITE, m_param);

	if (dma_buf && geni_se_tx_dma_prep(se, dma_buf, len, &tx_dma)) {
		geni_se_select_mode(se, GENI_SE_FIFO);
		i2c_put_dma_safe_msg_buf(dma_buf, msg, false);
		dma_buf = NULL;
	} else {
		gi2c->xfer_len = len;
		gi2c->dma_addr = tx_dma;
		gi2c->dma_buf = dma_buf;
	}

	if (!dma_buf) /* Get FIFO IRQ */
		writel_relaxed(1, se->base + SE_GENI_TX_WATERMARK_REG);

	cur = gi2c->cur;
	time_left = wait_for_completion_timeout(&gi2c->done, XFER_TIMEOUT);
	if (!time_left || (gi2c->err && gi2c->err != gi2c_log[ADDR_NACK].err))
		geni_i2c_cancel_xfer(gi2c);

	geni_i2c_tx_msg_cleanup(gi2c, cur);

	return gi2c->err;
}
/* …（同文件无关代码省略）… */
static void i2c_gpi_cb_result(void *cb, const struct dmaengine_result *result)
{
	struct geni_i2c_dev *gi2c = cb;
	struct geni_i2c_gpi_multi_desc_xfer *tx_multi_xfer;

	if (result->result != DMA_TRANS_NOERROR) {
		dev_err(gi2c->se.dev, "DMA txn failed:%d\n", result->result);
		gi2c->err = -EIO;
	} else if (result->residue) {
		dev_dbg(gi2c->se.dev, "DMA xfer has pending: %d\n", result->residue);
	}

	if (gi2c->is_tx_multi_desc_xfer) {
		tx_multi_xfer = &gi2c->i2c_multi_desc_config;
		tx_multi_xfer->irq_cnt++;
	}

	complete(&gi2c->done);
}
/* …（同文件无关代码省略）… */
static void geni_i2c_gpi_unmap(struct geni_i2c_dev *gi2c, struct i2c_msg *msg,
			       void *tx_buf, dma_addr_t tx_addr,
			       void *rx_buf, dma_addr_t rx_addr)
{
	if (tx_buf) {
		dma_unmap_single(gi2c->se.dev->parent, tx_addr, msg->len, DMA_TO_DEVICE);
		i2c_put_dma_safe_msg_buf(tx_buf, msg, !gi2c->err);
	}

	if (rx_buf) {
		dma_unmap_single(gi2c->se.dev->parent, rx_addr, msg->len, DMA_FROM_DEVICE);
		i2c_put_dma_safe_msg_buf(rx_buf, msg, !gi2c->err);
	}
}
/* …（同文件无关代码省略）… */
static void geni_i2c_gpi_multi_desc_unmap(struct geni_i2c_dev *gi2c, struct i2c_msg msgs[],
					  struct gpi_i2c_config *peripheral)
{
	u32 msg_xfer_cnt, wr_idx = 0;
	struct geni_i2c_gpi_multi_desc_xfer *tx_multi_xfer = &gi2c->i2c_multi_desc_config;

	msg_xfer_cnt = gi2c->err ? tx_multi_xfer->msg_idx_cnt : tx_multi_xfer->irq_cnt;

	/* Unmap the processed DMA buffers based on the received interrupt count */
	for (; tx_multi_xfer->unmap_msg_cnt < msg_xfer_cnt; tx_multi_xfer->unmap_msg_cnt++) {
		wr_idx = tx_multi_xfer->unmap_msg_cnt;
		geni_i2c_gpi_unmap(gi2c, &msgs[wr_idx],
				   tx_multi_xfer->dma_buf[wr_idx],
				   tx_multi_xfer->dma_addr[wr_idx],
				   NULL, 0);

		if (tx_multi_xfer->unmap_msg_cnt == gi2c->num_msgs - 1) {
			kfree(tx_multi_xfer->dma_buf);
			kfree(tx_multi_xfer->dma_addr);
			break;
		}
	}
}
/* …（同文件无关代码省略）… */
static int geni_i2c_gpi_multi_xfer_timeout_handler(struct device *dev,
						   struct geni_i2c_gpi_multi_desc_xfer *multi_xfer,
						   u32 transfer_timeout_msecs,
						   struct completion *transfer_comp)
{
	int i;
	u32 time_left;

	for (i = 0; i < multi_xfer->msg_idx_cnt - 1; i++) {
		reinit_completion(transfer_comp);

		if (multi_xfer->msg_idx_cnt != multi_xfer->irq_cnt) {
			time_left = wait_for_completion_timeout(transfer_comp,
								transfer_timeout_msecs);
			if (!time_left) {
				dev_err(dev, "%s: Transfer timeout\n", __func__);
				return -ETIMEDOUT;
			}
		}
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static int geni_i2c_gpi(struct geni_i2c_dev *gi2c, struct i2c_msg msgs[],
			struct dma_slave_config *config, dma_addr_t *dma_addr_p,
			void **buf, unsigned int op, struct dma_chan *dma_chan)
{
	struct gpi_i2c_config *peripheral;
	unsigned int flags;
	void *dma_buf = NULL;
	dma_addr_t addr = 0;
	enum dma_data_direction map_dirn;
	enum dma_transfer_direction dma_dirn;
	struct dma_async_tx_descriptor *desc;
	int ret;
	struct geni_i2c_gpi_multi_desc_xfer *gi2c_gpi_xfer;
	dma_cookie_t cookie;
	u32 msg_idx;

	peripheral = config->peripheral_config;
	gi2c_gpi_xfer = &gi2c->i2c_multi_desc_config;
	msg_idx = gi2c_gpi_xfer->msg_idx_cnt;

	/*
	 * Skip TX DMA mapping for a read message (I2C_M_RD) to avoid
	 * programming an extra TX DMA TRE that would cause an unintended
	 * write cycle on the I2C bus before the actual read operation.
	 */
	if (op == I2C_WRITE && msgs[msg_idx].flags & I2C_M_RD) {
		peripheral->multi_msg = true;
		goto skip_tx_dma_map;
	}

	dma_buf = i2c_get_dma_safe_msg_buf(&msgs[msg_idx], 1);
	if (!dma_buf) {
		ret = -ENOMEM;
		goto out;
	}

	if (op == I2C_WRITE)
		map_dirn = DMA_TO_DEVICE;
	else
		map_dirn = DMA_FROM_DEVICE;

	addr = dma_map_single(gi2c->se.dev->parent, dma_buf,
			      msgs[msg_idx].len, map_dirn);
	if (dma_mapping_error(gi2c->se.dev->parent, addr)) {
		i2c_put_dma_safe_msg_buf(dma_buf, &msgs[msg_idx], false);
		ret = -ENOMEM;
		goto out;
	}

skip_tx_dma_map:
	if (gi2c->is_tx_multi_desc_xfer) {
		flags = DMA_CTRL_ACK;

		/* BEI bit to be cleared for last TRE */
		if (msg_idx == gi2c->num_msgs - 1)
			flags |= DMA_PREP_INTERRUPT;
	} else {
		flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK;
	}

	/* set the length as message for rx txn */
	peripheral->rx_len = msgs[msg_idx].len;
	peripheral->op = op;

	ret = dmaengine_slave_config(dma_chan, config);
	if (ret) {
		dev_err(gi2c->se.dev, "dma config error: %d for op:%d\n", ret, op);
		goto err_config;
	}

	peripheral->set_config = 0;
	peripheral->multi_msg = true;

	if (op == I2C_WRITE)
		dma_dirn = DMA_MEM_TO_DEV;
	else
		dma_dirn = DMA_DEV_TO_MEM;

	desc = dmaengine_prep_slave_single(dma_chan, addr, msgs[msg_idx].len,
					   dma_dirn, flags);
	if (!desc && !(flags & DMA_PREP_INTERRUPT)) {
		/* Retry with interrupt if not enough TREs */
		flags |= DMA_PREP_INTERRUPT;
		desc = dmaengine_prep_slave_single(dma_chan, addr, msgs[msg_idx].len,
						   dma_dirn, flags);
	}

	if (!desc) {
		dev_err(gi2c->se.dev, "prep_slave_sg failed\n");
		ret = -EIO;
		goto err_config;
	}

	desc->callback_result = i2c_gpi_cb_result;
	desc->callback_param = gi2c;

	if (!((msgs[msg_idx].flags & I2C_M_RD) && op == I2C_WRITE))
		gi2c_gpi_xfer->msg_idx_cnt++;

	cookie = dmaengine_submit(desc);
	if (dma_submit_error(cookie)) {
		dev_err(gi2c->se.dev,
			"%s: dmaengine_submit failed (%d)\n", __func__, cookie);
		ret = -EINVAL;
		goto err_config;
	}

	if (gi2c->is_tx_multi_desc_xfer) {
		gi2c_gpi_xfer->dma_buf[msg_idx] = dma_buf;
		gi2c_gpi_xfer->dma_addr[msg_idx] = addr;

		dma_async_issue_pending(gi2c->tx_c);

		if ((msg_idx == (gi2c->num_msgs - 1)) || flags & DMA_PREP_INTERRUPT) {
			ret = geni_i2c_gpi_multi_xfer_timeout_handler(gi2c->se.dev, gi2c_gpi_xfer,
								      XFER_TIMEOUT, &gi2c->done);
			if (ret) {
				dev_err(gi2c->se.dev,
					"I2C multi write msg transfer timeout: %d\n",
					ret);
				gi2c->err = ret;
				return ret;
			}
		}
	} else {
		/* Non multi descriptor message transfer */
		*buf = dma_buf;
		*dma_addr_p = addr;
	}
	return 0;

err_config:
	/* Avoid DMA unmap as the write operation skipped DMA mapping */
	if (dma_buf) {
		dma_unmap_single(gi2c->se.dev->parent, addr,
				 msgs[msg_idx].len, map_dirn);
		i2c_put_dma_safe_msg_buf(dma_buf, &msgs[msg_idx], false);
	}

out:
	gi2c->err = ret;
	return ret;
}
/* …（同文件无关代码省略）… */
static int geni_i2c_gpi_xfer(struct geni_i2c_dev *gi2c, struct i2c_msg msgs[], int num)
{
	struct dma_slave_config config = {};
	struct gpi_i2c_config peripheral = {};
	int i, ret = 0;
	unsigned long time_left;
	dma_addr_t tx_addr, rx_addr;
	void *tx_buf = NULL, *rx_buf = NULL;
	struct geni_i2c_gpi_multi_desc_xfer *tx_multi_xfer;
	const struct geni_i2c_clk_fld *itr = gi2c->clk_fld;

	config.peripheral_config = &peripheral;
	config.peripheral_size = sizeof(peripheral);

	peripheral.pack_enable = I2C_PACK_TX | I2C_PACK_RX;
	peripheral.cycle_count = itr->t_cycle_cnt;
	peripheral.high_count = itr->t_high_cnt;
	peripheral.low_count = itr->t_low_cnt;
	peripheral.clk_div = itr->clk_div;
	peripheral.set_config = 1;
	peripheral.multi_msg = false;

	trace_geni_i2c_bus_setup(gi2c->se.dev, gi2c->clk_freq_out,
				 itr->clk_div, itr->t_high_cnt,
				 itr->t_low_cnt, itr->t_cycle_cnt);

	gi2c->num_msgs = num;
	gi2c->is_tx_multi_desc_xfer = false;

	tx_multi_xfer = &gi2c->i2c_multi_desc_config;
	memset(tx_multi_xfer, 0, sizeof(struct geni_i2c_gpi_multi_desc_xfer));

	/*
	 * If number of write messages are two and higher then
	 * configure hardware for multi descriptor transfers with BEI.
	 */
	if (num >= QCOM_I2C_MIN_NUM_OF_MSGS_MULTI_DESC) {
		gi2c->is_tx_multi_desc_xfer = true;
		for (i = 0; i < num; i++) {
			if (msgs[i].flags & I2C_M_RD) {
				/*
				 * Multi descriptor transfer with BEI
				 * support is enabled for write transfers.
				 * TODO: Add BEI optimization support for
				 * read transfers later.
				 */
				gi2c->is_tx_multi_desc_xfer = false;
				break;
			}
		}
	}

	if (gi2c->is_tx_multi_desc_xfer) {
		tx_multi_xfer->dma_buf = kcalloc(num, sizeof(void *), GFP_KERNEL);
		tx_multi_xfer->dma_addr = kzalloc_objs(dma_addr_t, num);
		if (!tx_multi_xfer->dma_buf || !tx_multi_xfer->dma_addr) {
			ret = -ENOMEM;
			goto err;
		}
	}

	for (i = 0; i < num; i++) {
		gi2c->cur = &msgs[i];
		gi2c->err = 0;
		reinit_completion(&gi2c->done);
		dev_dbg(gi2c->se.dev, "msg[%d].len:%d\n", i, gi2c->cur->len);

		peripheral.stretch = 0;
		if (i < num - 1)
			peripheral.stretch = 1;

		peripheral.addr = msgs[i].addr;
		if (i > 0 && (!(msgs[i].flags & I2C_M_RD)))
			peripheral.multi_msg = false;

		ret =  geni_i2c_gpi(gi2c, msgs, &config,
				    &tx_addr, &tx_buf, I2C_WRITE, gi2c->tx_c);
		if (ret)
			goto err;

		if (msgs[i].flags & I2C_M_RD) {
			ret =  geni_i2c_gpi(gi2c, msgs, &config,
					    &rx_addr, &rx_buf, I2C_READ, gi2c->rx_c);
			if (ret)
				goto err;

			dma_async_issue_pending(gi2c->rx_c);
		}

		if (!gi2c->is_tx_multi_desc_xfer) {
			dma_async_issue_pending(gi2c->tx_c);
			time_left = wait_for_completion_timeout(&gi2c->done, XFER_TIMEOUT);
			if (!time_left) {
				dev_err(gi2c->se.dev, "%s:I2C timeout\n", __func__);
				gi2c->err = -ETIMEDOUT;
			}
		}

		if (gi2c->err) {
			ret = gi2c->err;
			goto err;
		}

		if (!gi2c->is_tx_multi_desc_xfer)
			geni_i2c_gpi_unmap(gi2c, &msgs[i], tx_buf, tx_addr, rx_buf, rx_addr);
		else if (tx_multi_xfer->unmap_msg_cnt != tx_multi_xfer->irq_cnt)
			geni_i2c_gpi_multi_desc_unmap(gi2c, msgs, &peripheral);
	}

	return num;

err:
	dev_err(gi2c->se.dev, "GPI transfer failed: %d\n", ret);
	dmaengine_terminate_sync(gi2c->rx_c);
	dmaengine_terminate_sync(gi2c->tx_c);
	if (gi2c->is_tx_multi_desc_xfer)
		geni_i2c_gpi_multi_desc_unmap(gi2c, msgs, &peripheral);
	else
		geni_i2c_gpi_unmap(gi2c, &msgs[i], tx_buf, tx_addr, rx_buf, rx_addr);

	return ret;
}
/* …（同文件无关代码省略）… */
static int geni_i2c_fifo_xfer(struct geni_i2c_dev *gi2c,
			      struct i2c_msg msgs[], int num)
{
	int i, ret = 0;

	for (i = 0; i < num; i++) {
		u32 m_param = i < (num - 1) ? STOP_STRETCH : 0;

		m_param |= ((msgs[i].addr << SLV_ADDR_SHFT) & SLV_ADDR_MSK);

		gi2c->cur = &msgs[i];
		gi2c->err = 0;
		reinit_completion(&gi2c->done);
		if (msgs[i].flags & I2C_M_RD)
			ret = geni_i2c_rx_one_msg(gi2c, &msgs[i], m_param);
		else
			ret = geni_i2c_tx_one_msg(gi2c, &msgs[i], m_param);

		if (ret)
			return ret;
	}

	return num;
}
/* …（同文件无关代码省略）… */

	ret = gi2c->dev_data->set_rate(&gi2c->se, gi2c->clk_freq_out);
	if (ret)
		return ret;

	if (gi2c->gpi_mode)
		ret = geni_i2c_gpi_xfer(gi2c, msgs, num);
	else
		ret = geni_i2c_fifo_xfer(gi2c, msgs, num);

	pm_runtime_put_autosuspend(gi2c->se.dev);
	gi2c->cur = NULL;
	gi2c->err = 0;
