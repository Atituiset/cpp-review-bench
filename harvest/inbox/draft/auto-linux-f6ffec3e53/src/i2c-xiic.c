// AUTO-DRAFT from torvalds/linux PR #7704c4c5bb127673b4f0ead839919db573559e38
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define SMBUS_BLOCK_READ_MIN_LEN	3
/* …（同文件无关代码省略）… */
enum xilinx_i2c_state {
	STATE_DONE,
	STATE_ERROR,
	STATE_START
};
/* …（同文件无关代码省略）… */
enum xiic_endian {
	LITTLE,
	BIG
};
/* …（同文件无关代码省略）… */
 * @prev_msg_tx: Previous message is Tx
 * @quirks: To hold platform specific bug info
 * @smbus_block_read: Flag to handle block read
 * @input_clk: Input clock to I2C controller
 * @i2c_clk: I2C SCL frequency
 * @atomic: Mode of transfer
/* …（同文件无关代码省略）… */
struct xiic_i2c {
	struct device *dev;
	void __iomem *base;
	struct completion completion;
	struct i2c_adapter adap;
	struct i2c_msg *tx_msg;
	struct mutex lock;
	unsigned int tx_pos;
	unsigned int nmsgs;
	struct i2c_msg *rx_msg;
	int rx_pos;
	enum xiic_endian endianness;
	struct clk *clk;
	enum xilinx_i2c_state state;
	bool singlemaster;
	bool dynamic;
	bool prev_msg_tx;
	u32 quirks;
	bool smbus_block_read;
	unsigned long input_clk;
	unsigned int i2c_clk;
	bool atomic;
	spinlock_t atomic_lock;		/* Lock for atomic transfer mode */
	enum xilinx_i2c_state atomic_xfer_state;
};
/* …（同文件无关代码省略）… */
#define XIIC_MSB_OFFSET 0
#define XIIC_REG_OFFSET (0x100 + XIIC_MSB_OFFSET)
/* …（同文件无关代码省略）… */
#define XIIC_RFD_REG_OFFSET  (0x20 + XIIC_REG_OFFSET)	/* Rx FIFO Depth reg  */
/* …（同文件无关代码省略）… */
#define XIIC_INTR_TX_ERROR_MASK           0x02	/* 1=Tx error/msg complete */
/* …（同文件无关代码省略）… */
#define IIC_RX_FIFO_DEPTH         16	/* Rx fifo capacity               */
/* …（同文件无关代码省略）… */
#define XIIC_IISR_OFFSET     0x20 /* Interrupt Status Register */
#define XIIC_IIER_OFFSET     0x28 /* Interrupt Enable Register */
/* …（同文件无关代码省略）… */
#define xiic_rx_space(i2c) ((i2c)->rx_msg->len - (i2c)->rx_pos)
/* …（同文件无关代码省略）… */
static inline void xiic_setreg8(struct xiic_i2c *i2c, int reg, u8 value)
{
	if (i2c->endianness == LITTLE)
		iowrite8(value, i2c->base + reg);
	else
		iowrite8(value, i2c->base + reg + 3);
}
/* …（同文件无关代码省略）… */
static inline void xiic_setreg32(struct xiic_i2c *i2c, int reg, int value)
{
	if (i2c->endianness == LITTLE)
		iowrite32(value, i2c->base + reg);
	else
		iowrite32be(value, i2c->base + reg);
}
/* …（同文件无关代码省略）… */
static inline int xiic_getreg32(struct xiic_i2c *i2c, int reg)
{
	u32 ret;

	if (i2c->endianness == LITTLE)
		ret = ioread32(i2c->base + reg);
	else
		ret = ioread32be(i2c->base + reg);
	return ret;
}
/* …（同文件无关代码省略）… */
static inline void xiic_irq_en(struct xiic_i2c *i2c, u32 mask)
{
	u32 ier = xiic_getreg32(i2c, XIIC_IIER_OFFSET);

	xiic_setreg32(i2c, XIIC_IIER_OFFSET, ier | mask);
}
/* …（同文件无关代码省略）… */
static inline void xiic_irq_clr(struct xiic_i2c *i2c, u32 mask)
{
	u32 isr = xiic_getreg32(i2c, XIIC_IISR_OFFSET);

	xiic_setreg32(i2c, XIIC_IISR_OFFSET, isr & mask);
}
/* …（同文件无关代码省略）… */
static inline void xiic_irq_clr_en(struct xiic_i2c *i2c, u32 mask)
{
	xiic_irq_clr(i2c, mask);
	xiic_irq_en(i2c, mask);
}
/* …（同文件无关代码省略）… */

	/* Check if received length is valid */
	if (rxmsg_len <= I2C_SMBUS_BLOCK_MAX) {
		/* Set Receive fifo depth */
		if (rxmsg_len > IIC_RX_FIFO_DEPTH) {
			/*
			 * When Rx msg len greater than or equal to Rx fifo capacity
			 * Receive fifo depth should set to Rx fifo capacity minus 1
			 */
			rfd_set = IIC_RX_FIFO_DEPTH - 1;
			i2c->rx_msg->len = rxmsg_len + 1;
		} else if ((rxmsg_len == 1) ||
			(rxmsg_len == 0)) {
			/*
			 * Minimum of 3 bytes required to exit cleanly. 1 byte
			 * already received, Second byte is being received. Have
			 * to set NACK in read_rx before receiving the last byte
			 */
			rfd_set = 0;
			i2c->rx_msg->len = SMBUS_BLOCK_READ_MIN_LEN;
		} else {
			/*
			 * When Rx msg len less than Rx fifo capacity
			 * Receive fifo depth should set to Rx msg len minus 2
			 */
			rfd_set = rxmsg_len - 2;
			i2c->rx_msg->len = rxmsg_len + 1;
		}
		xiic_setreg8(i2c, XIIC_RFD_REG_OFFSET, rfd_set);

/* …（同文件无关代码省略）… */
	dev_err(i2c->adap.dev.parent, "smbus_block_read Invalid msg length\n");
}

static void xiic_read_rx(struct xiic_i2c *i2c)
{
	u8 bytes_in_fifo, cr = 0, bytes_to_read = 0;
/* …（同文件无关代码省略）… */

		xiic_read_rx(i2c);
		if (xiic_rx_space(i2c) == 0) {
			/* this is the last part of the message */
			i2c->rx_msg = NULL;

/* …（同文件无关代码省略）… */

		if (i2c->tx_msg && i2c->smbus_block_read) {
			i2c->smbus_block_read = false;
			/* Set requested message len=1 to indicate STATE_DONE */
			i2c->tx_msg->len = 1;
		}

		if (!i2c->tx_msg)
/* …（同文件无关代码省略）… */
			return;
	}

	i2c->rx_msg = NULL;
	xiic_irq_clr_en(i2c, XIIC_INTR_TX_ERROR_MASK);

/* …（同文件无关代码省略）… */
	u8 cr = 0, rfd_set = 0;
	struct i2c_msg *msg = i2c->rx_msg = i2c->tx_msg;

	if (!i2c->atomic)
		dev_dbg(i2c->adap.dev.parent, "%s entry, ISR: 0x%x, CR: 0x%x\n",
			__func__, xiic_getreg32(i2c, XIIC_IISR_OFFSET),
