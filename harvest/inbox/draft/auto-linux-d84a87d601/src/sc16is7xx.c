// AUTO-DRAFT from torvalds/linux PR #9a32e0754d637c1d386a533ae36e41c8f4be8b10
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define SC16IS7XX_THR_REG		(0x00) /* TX FIFO */
/* …（同文件无关代码省略）… */
#define SC16IS7XX_MCR_REG		(0x04) /* Modem Control */
/* …（同文件无关代码省略）… */
#define SC16IS7XX_TXLVL_REG		(0x08) /* TX FIFO level */
/* …（同文件无关代码省略）… */
#define SC16IS7XX_MCR_TCRTLR_BIT	BIT(2)   /* TCR/TLR registers enable */
/* …（同文件无关代码省略）… */
#define SC16IS7XX_TCR_RX_HALT(words)	((((words) / 4) & 0x0f) << 0)
#define SC16IS7XX_TCR_RX_RESUME(words)	((((words) / 4) & 0x0f) << 4)
/* …（同文件无关代码省略）… */
#define SC16IS7XX_TLR_TX_TRIGGER(words)	((((words) / 4) & 0x0f) << 0)
#define SC16IS7XX_TLR_RX_TRIGGER(words)	((((words) / 4) & 0x0f) << 4)

/* IOControl register bits (Only 75x/76x) */
#define SC16IS7XX_IOCONTROL_LATCH_BIT	BIT(0)   /* Enable input latching */
#define SC16IS7XX_IOCONTROL_MODEM_A_BIT	BIT(1)   /* Enable GPIO[7:4] as modem A pins */
/* …（同文件无关代码省略）… */
#define SC16IS7XX_FIFO_SIZE		(64)
/* …（同文件无关代码省略）… */
struct sc16is7xx_one_config {
	unsigned int			flags;
	u8				ier_mask;
	u8				ier_val;
};
/* …（同文件无关代码省略）… */
struct sc16is7xx_one {
	struct uart_port		port;
	struct regmap			*regmap;
	struct mutex			lock; /* For registers sharing same address space. */
	struct kthread_work		tx_work;
	struct kthread_work		reg_work;
	struct kthread_delayed_work	ms_work;
	struct sc16is7xx_one_config	config;
	unsigned char			buf[SC16IS7XX_FIFO_SIZE]; /* Rx buffer. */
	unsigned int			old_mctrl;
	u8				old_lcr; /* Value before EFR access. */
	bool				irda_mode;
};
/* …（同文件无关代码省略）… */
#define to_sc16is7xx_one(p)	container_of((p), struct sc16is7xx_one, port)
/* …（同文件无关代码省略）… */
static u8 sc16is7xx_port_read(struct uart_port *port, u8 reg)
{
	struct sc16is7xx_one *one = to_sc16is7xx_one(port);
	unsigned int val = 0;

	regmap_read(one->regmap, reg, &val);

	return val;
}
/* …（同文件无关代码省略）… */
static void sc16is7xx_port_write(struct uart_port *port, u8 reg, u8 val)
{
	struct sc16is7xx_one *one = to_sc16is7xx_one(port);

	regmap_write(one->regmap, reg, val);
}
/* …（同文件无关代码省略）… */
static void sc16is7xx_fifo_write(struct uart_port *port, u8 *txbuf, u8 to_send)
{
	struct sc16is7xx_one *one = to_sc16is7xx_one(port);

	/*
	 * Don't send zero-length data, at least on SPI it confuses the chip
	 * delivering wrong TXLVL data.
	 */
	if (unlikely(!to_send))
		return;

	regmap_noinc_write(one->regmap, SC16IS7XX_THR_REG, txbuf, to_send);
}
/* …（同文件无关代码省略）… */
static void sc16is7xx_port_update(struct uart_port *port, u8 reg,
				  u8 mask, u8 val)
{
	struct sc16is7xx_one *one = to_sc16is7xx_one(port);

	regmap_update_bits(one->regmap, reg, mask, val);
}
/* …（同文件无关代码省略）… */
	tty_flip_buffer_push(&port->state->port);
}

static void sc16is7xx_handle_tx(struct uart_port *port)
{
	struct tty_port *tport = &port->state->port;
	unsigned long flags;
	unsigned int txlen;
	unsigned char *tail;

	if (unlikely(port->x_char)) {
		sc16is7xx_port_write(port, SC16IS7XX_THR_REG, port->x_char);
/* …（同文件无关代码省略）… */
	}

	/* Limit to space available in TX FIFO */
	txlen = sc16is7xx_port_read(port, SC16IS7XX_TXLVL_REG);
	if (txlen > SC16IS7XX_FIFO_SIZE) {
		dev_err_ratelimited(port->dev,
			"chip reports %d free bytes in TX fifo, but it only has %d",
			txlen, SC16IS7XX_FIFO_SIZE);
		txlen = 0;
	}

	txlen = kfifo_out_linear_ptr(&tport->xmit_fifo, &tail, txlen);
	sc16is7xx_fifo_write(port, tail, txlen);
	uart_xmit_advance(port, txlen);

	uart_port_lock_irqsave(port, &flags);
	if (kfifo_len(&tport->xmit_fifo) < WAKEUP_CHARS)
/* …（同文件无关代码省略）… */
			     SC16IS7XX_TCR_RX_RESUME(24) |
			     SC16IS7XX_TCR_RX_HALT(48));

	/* Disable TCR/TLR access */
	sc16is7xx_port_update(port, SC16IS7XX_MCR_REG, SC16IS7XX_MCR_TCRTLR_BIT, 0);
