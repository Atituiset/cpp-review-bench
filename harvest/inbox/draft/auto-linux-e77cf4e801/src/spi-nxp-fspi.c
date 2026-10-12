// AUTO-DRAFT from torvalds/linux PR #f47959816751f02a711aab27cf1c805bd4964819
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define FSPI_MCR0			0x00
/* …（同文件无关代码省略）… */
#define FSPI_MCR0_MDIS			BIT(1)
/* …（同文件无关代码省略）… */
#define FSPI_DLLACR			0xC0
#define FSPI_DLLACR_OVRDEN		BIT(8)
/* …（同文件无关代码省略）… */
#define FSPI_DLLBCR			0xC4
#define FSPI_DLLBCR_OVRDEN		BIT(8)
/* …（同文件无关代码省略）… */
struct nxp_fspi_devtype_data {
	unsigned int rxfifo;
	unsigned int txfifo;
	unsigned int ahb_buf_size;
	unsigned int quirks;
	unsigned int lut_num;
	bool little_endian;
	/*
	 * The max clock rate (Hz) that FlexSPI can output to the device
	 * in SDR mode (RXCLKSRC=0). Defaults to 66MHz if zero.
	 * Some SoCs (e.g. LX2160A) support up to 100MHz in SDR mode.
	 */
	unsigned long max_sdr_rate;
	/*
	 * The max clock rate (Hz) that FlexSPI can output to the device
	 * in DTR mode (RXCLKSRC=3). Defaults to 166MHz if zero.
	 * Some SoCs (e.g. i.MX95, i.MX8QM, i.MX8DXL) support up to 200MHz.
	 */
	unsigned long max_dtr_rate;
};
/* …（同文件无关代码省略）… */
struct nxp_fspi {
	void __iomem *iobase;
	void __iomem *ahb_addr;
	u32 memmap_phy;
	u32 memmap_phy_size;
	u32 memmap_start;
	u32 memmap_len;
	struct clk *clk, *clk_en;
	struct device *dev;
	struct completion c;
	struct nxp_fspi_devtype_data *devtype_data;
	struct mutex lock;
	struct pm_qos_request pm_qos_req;
	int selected;
#define FSPI_NEED_INIT		BIT(0)
#define FSPI_DTR_MODE		BIT(1)
	int flags;
	/* save the previous operation clock rate */
	unsigned long pre_op_rate;
	/* the max clock rate fspi output to device */
	unsigned long max_rate;
};
/* …（同文件无关代码省略）… */
static void fspi_writel(struct nxp_fspi *f, u32 val, void __iomem *addr)
{
	if (f->devtype_data->little_endian)
		iowrite32(val, addr);
	else
		iowrite32be(val, addr);
}
/* …（同文件无关代码省略）… */
static u32 fspi_readl(struct nxp_fspi *f, void __iomem *addr)
{
	if (f->devtype_data->little_endian)
		return ioread32(addr);
	else
		return ioread32be(addr);
}
/* …（同文件无关代码省略）… */
static void nxp_fspi_dll_override(struct nxp_fspi *f)
{
	fspi_writel(f, FSPI_DLLACR_OVRDEN, f->iobase + FSPI_DLLACR);
	fspi_writel(f, FSPI_DLLBCR_OVRDEN, f->iobase + FSPI_DLLBCR);
}
/* …（同文件无关代码省略）… */
	dev_dbg(f->dev, "Target device [CS:%x] selected\n", spi_get_chipselect(spi, 0));

	/*
	 * Per the FlexSPI reference manual (initialization sequence), MCR0 and
	 * the DLL control registers should be configured while the module is in
	 * stop mode (MCR0[MDIS] = 1). Enter stop mode before reconfiguring the
	 * RX sample clock source and the DLL, then exit stop mode afterwards.
	 */
	reg = fspi_readl(f, f->iobase + FSPI_MCR0);
	fspi_writel(f, reg | FSPI_MCR0_MDIS, f->iobase + FSPI_MCR0);
/* …（同文件无关代码省略）… */
	if (ret)
		return ret;

	/*
	 * If clock rate > 100MHz, then switch from DLL override mode to
	 * DLL calibration mode.
/* …（同文件无关代码省略）… */
	else
		nxp_fspi_dll_override(f);

	/* Exit stop mode now that MCR0 and the DLL have been reconfigured. */
	reg = fspi_readl(f, f->iobase + FSPI_MCR0);
	fspi_writel(f, reg & ~FSPI_MCR0_MDIS, f->iobase + FSPI_MCR0);

	f->pre_op_rate = op->max_freq;

	f->selected = spi_get_chipselect(spi, 0);
