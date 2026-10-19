// AUTO-DRAFT from torvalds/linux PR #72732cbee632b1be22b636813c9be3df55c72bd9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
  // <<< BUG ANCHOR
#define GENI_SE_CLK_32MHZ	(32 * HZ_PER_MHZ)
#define GENI_SE_CLK_19P2MHZ	19200000UL
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
static int geni_i2c_clk_map_idx(struct geni_i2c_dev *gi2c)
{
	const struct geni_i2c_clk_fld *itr;
	unsigned long res_freq;

	/*
	 * Frequency counter tables are calibrated for a specific source
	 * clock frequency and are not valid for any multiple of it
	 * (e.g. 64 MHz, 128 MHz).
	 * Use exact=true and verify res_freq matches req_freq literally
	 * to reject harmonics: a 64 MHz clock that divides evenly to
	 * 32 MHz would pass exact matching but produce double the intended
	 * I2C frequency with these counter values.
	 */
	if (!geni_se_clk_freq_match(&gi2c->se, GENI_SE_CLK_32MHZ,
				    &gi2c->clk_idx, &res_freq, true) &&
	    res_freq == GENI_SE_CLK_32MHZ) {
		itr = geni_i2c_clk_map_32mhz;
	} else if (!geni_se_clk_freq_match(&gi2c->se, GENI_SE_CLK_19P2MHZ,
					   &gi2c->clk_idx, &res_freq, true) &&
		   res_freq == GENI_SE_CLK_19P2MHZ) {
		itr = geni_i2c_clk_map_19p2mhz;
	} else {
		dev_err(gi2c->se.dev,
			"Unsupported SE source clock: must be exactly 32 MHz or 19.2 MHz\n");
		return -EINVAL;
	}
