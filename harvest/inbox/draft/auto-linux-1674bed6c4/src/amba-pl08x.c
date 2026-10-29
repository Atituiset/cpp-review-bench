// AUTO-DRAFT from torvalds/linux PR #ae3690eb2a994dd74a43639d6bf800b51f18b48c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
struct vendor_data {
	u8 config_offset;
	u8 channels;
	u8 signals;
	bool dualmaster;
	bool nomadik;
	bool pl080s;
	bool ftdmac020;
	u32 max_transfer_size;
};
/* …（同文件无关代码省略）… */
struct pl08x_phy_chan {
	unsigned int id;
	void __iomem *base;
	void __iomem *reg_config;
	void __iomem *reg_control;
	void __iomem *reg_src;
	void __iomem *reg_dst;
	void __iomem *reg_lli;
	void __iomem *reg_busy;
	spinlock_t lock;
	struct pl08x_dma_chan *serving;
	bool locked;
	bool ftdmac020;
	bool pl080s;
};
/* …（同文件无关代码省略）… */
struct pl08x_txd {
	struct virt_dma_desc vd;
	struct list_head dsg_list;
	dma_addr_t llis_bus;
	u32 *llis_va;
	/* Default cctl value for LLIs */
	u32 cctl;
	/*
	 * Settings to be put into the physical channel when we
	 * trigger this txd.  Other registers are in llis_va[0].
	 */
	u32 ccfg;
	bool done;
	bool cyclic;
};
/* …（同文件无关代码省略）… */
enum pl08x_dma_chan_state {
	PL08X_CHAN_IDLE,
	PL08X_CHAN_RUNNING,
	PL08X_CHAN_PAUSED,
	PL08X_CHAN_WAITING,
};
/* …（同文件无关代码省略）… */
struct pl08x_dma_chan {
	struct virt_dma_chan vc;
	struct pl08x_phy_chan *phychan;
	const char *name;
	struct pl08x_channel_data *cd;
	struct dma_slave_config cfg;
	struct pl08x_txd *at;
	struct pl08x_driver_data *host;
	enum pl08x_dma_chan_state state;
	bool slave;
	int signal;
	unsigned mux_use;
	unsigned long waiting_at;
};
/* …（同文件无关代码省略）… */
struct pl08x_driver_data {
	struct dma_device slave;
	struct dma_device memcpy;
	bool has_slave;
	void __iomem *base;
	struct amba_device *adev;
	const struct vendor_data *vd;
	struct pl08x_platform_data *pd;
	struct pl08x_phy_chan *phy_chans;
	struct dma_pool *pool;
	u8 lli_buses;
	u8 mem_buses;
	u8 lli_words;
};
/* …（同文件无关代码省略）… */
static void init_pl08x_debugfs(struct pl08x_driver_data *pl08x)
{
	/* Expose a simple debugfs interface to view all clocks */
	debugfs_create_file(dev_name(&pl08x->adev->dev), S_IFREG | S_IRUGO,
			    NULL, pl08x, &pl08x_debugfs_fops);
}

#else
