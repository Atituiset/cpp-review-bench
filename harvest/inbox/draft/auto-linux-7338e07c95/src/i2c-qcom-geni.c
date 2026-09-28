// AUTO-DRAFT from torvalds/linux PR #64d34cef2a32331fedabf25ea8fa8a30128af9f7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
  // <<< BUG ANCHOR
#define XFER_TIMEOUT		HZ
#define RST_TIMEOUT		HZ

struct geni_i2c_desc {
	bool no_dma_support;
	unsigned int tx_fifo_depth;
/* …（同文件无关代码省略）… */
struct geni_i2c_gpi_multi_desc_xfer {
	u32 msg_idx_cnt;
	u32 unmap_msg_cnt;
	u32 irq_cnt;
	void **dma_buf;
	dma_addr_t *dma_addr;
};
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
static int geni_i2c_clk_map_idx(struct geni_i2c_dev *gi2c)
{
	const struct geni_i2c_clk_fld *itr;

	if (clk_get_rate(gi2c->se.clk) == 32 * HZ_PER_MHZ)
		itr = geni_i2c_clk_map_32mhz;
	else
		itr = geni_i2c_clk_map_19p2mhz;

	while (itr->clk_freq_out != 0) {
		if (itr->clk_freq_out == gi2c->clk_freq_out) {
			gi2c->clk_fld = itr;
			return 0;
		}
		itr++;
	}
	return -EINVAL;
}

/* …（同文件无关代码省略）… */
	const struct geni_i2c_clk_fld *itr = gi2c->clk_fld;
	u32 val;

	writel_relaxed(0, gi2c->se.base + SE_GENI_CLK_SEL);

	val = (itr->clk_div << CLK_DIV_SHFT) | SER_CLK_EN;
	writel_relaxed(val, gi2c->se.base + GENI_SER_M_CLK_CFG);
/* …（同文件无关代码省略）… */

	ret = geni_i2c_clk_map_idx(gi2c);
	if (ret)
		return dev_err_probe(gi2c->se.dev, ret, "Invalid clk frequency %d Hz\n",
				     gi2c->clk_freq_out);

	return geni_icc_set_bw_ab(&gi2c->se, GENI_DEFAULT_BW, GENI_DEFAULT_BW,
				  Bps_to_icc(gi2c->clk_freq_out));
