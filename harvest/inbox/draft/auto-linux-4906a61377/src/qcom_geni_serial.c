// AUTO-DRAFT from torvalds/linux PR #9a32e0754d637c1d386a533ae36e41c8f4be8b10
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define SE_UART_IO_MACRO_CTRL		0x240
/* …（同文件无关代码省略）… */
#define SE_UART_TX_TRANS_LEN		0x270
/* …（同文件无关代码省略）… */
#define SE_UART_RX_STALE_CNT		0x294
/* …（同文件无关代码省略）… */
#define UART_START_TX			0x1
/* …（同文件无关代码省略）… */
#define STALE_TIMEOUT			16
#define DEFAULT_BITS_PER_CHAR		10
#define GENI_UART_CONS_PORTS		1
#define DEF_FIFO_DEPTH_WORDS		16
#define DEF_TX_WM			2
/* …（同文件无关代码省略）… */
#define UART_RX_WM			2
/* …（同文件无关代码省略）… */
#define DEFAULT_IO_MACRO_IO0_IO1_MASK	GENMASK(3, 0)
#define IO_MACRO_IO0_SEL		0x3
#define DEFAULT_IO_MACRO_IO2_IO3_MASK	GENMASK(15, 4)
#define IO_MACRO_IO2_IO3_SWAP		0x4640
/* …（同文件无关代码省略）… */
#define BYTES_PER_FIFO_WORD		4U
/* …（同文件无关代码省略）… */
struct qcom_geni_device_data {
	bool console;
	enum geni_se_xfer_mode mode;
	int (*resources_init)(struct geni_se *se);
	int (*set_rate)(struct geni_se *se, unsigned long baud);
	int (*power_on)(struct geni_se *se);
	int (*power_off)(struct geni_se *se);
};
/* …（同文件无关代码省略）… */
struct qcom_geni_private_data {
	/* NOTE: earlycon port will have NULL here */
	struct uart_driver *drv;

	u32 poll_cached_bytes;
	unsigned int poll_cached_bytes_cnt;

	u32 write_cached_bytes;
	unsigned int write_cached_bytes_cnt;
};
/* …（同文件无关代码省略）… */
struct qcom_geni_serial_port {
	struct uart_port uport;
	struct geni_se se;
	const char *name;
	u32 tx_fifo_depth;
	u32 tx_fifo_width;
	u32 rx_fifo_depth;
	dma_addr_t tx_dma_addr;
	dma_addr_t rx_dma_addr;
	bool setup;
	unsigned long poll_timeout_us;
	unsigned long clk_rate;
	void *rx_buf;
	u32 loopback;
	bool brk;

	unsigned int tx_remaining;
	unsigned int tx_queued;
	bool tx_dma_stale;
	int wakeup_irq;
	bool rx_tx_swap;
	bool cts_rts_swap;
	bool manual_flow;
	bool tx_kick_enabled;
	struct irq_work tx_kick;
	struct qcom_geni_private_data private_data;
	const struct qcom_geni_device_data *dev_data;
	struct notifier_block panic_nb;
};
/* …（同文件无关代码省略）… */
static int qcom_geni_serial_port_setup(struct uart_port *uport);
static void qcom_geni_serial_start_tx_fifo(struct uart_port *uport);
static void qcom_geni_serial_resume_tx(struct uart_port *uport);
static void qcom_geni_serial_poll_rx_fifo_locked(struct uart_port *uport);

static inline struct qcom_geni_serial_port *to_dev_port(struct uart_port *uport)
{
/* …（同文件无关代码省略）… */
static struct qcom_geni_serial_port *get_port_from_line(int line, bool console, struct device *dev)
{
	struct qcom_geni_serial_port *port;
	int nr_ports = console ? GENI_UART_CONS_PORTS : CONFIG_SERIAL_QCOM_GENI_UART_PORTS;

	if (console) {
		if (line < 0 || line >= nr_ports)
			return ERR_PTR(-ENXIO);

		port = &qcom_geni_console_port;
	} else {
		int max_alias_num = of_alias_get_highest_id("serial");

		if (line < 0 || line >= nr_ports)
			line = ida_alloc_range(&port_ida, max_alias_num + 1,
					       nr_ports - 1, GFP_KERNEL);
		else
			line = ida_alloc_range(&port_ida, line,
					       nr_ports - 1, GFP_KERNEL);

		if (line < 0)
			return ERR_PTR(-ENXIO);

		port = devm_kzalloc(dev, sizeof(*port), GFP_KERNEL);
		if (!port)
			return ERR_PTR(-ENOMEM);

		port->uport.iotype = UPIO_MEM;
		port->uport.ops = &qcom_geni_uart_pops;
		port->uport.flags = UPF_BOOT_AUTOCONF;
		port->uport.line = line;
	}
	return port;
}
/* …（同文件无关代码省略）… */
static bool qcom_geni_serial_main_active(struct uart_port *uport)
{
	return readl(uport->membase + SE_GENI_STATUS) & M_GENI_CMD_ACTIVE;
}
/* …（同文件无关代码省略）… */
static bool qcom_geni_serial_poll_bitfield(struct uart_port *uport,
					   unsigned int offset, u32 field, u32 val)
{
	u32 reg;
	struct qcom_geni_serial_port *port;
	unsigned long timeout_us = 20000;
	struct qcom_geni_private_data *private_data = uport->private_data;

	if (private_data->drv) {
		port = to_dev_port(uport);
		if (port->poll_timeout_us)
			timeout_us = port->poll_timeout_us;
	}

	/*
	 * Use custom implementation instead of readl_poll_atomic since ktimer
	 * is not ready at the time of early console.
	 */
	timeout_us = DIV_ROUND_UP(timeout_us, 10) * 10;
	while (timeout_us) {
		reg = readl(uport->membase + offset);
		if ((reg & field) == val)
			return true;
		udelay(10);
		timeout_us -= 10;
	}
	return false;
}
/* …（同文件无关代码省略）… */
static bool qcom_geni_serial_poll_bit(struct uart_port *uport,
				      unsigned int offset, u32 field, bool set)
{
	return qcom_geni_serial_poll_bitfield(uport, offset, field, set ? field : 0);
}
/* …（同文件无关代码省略）… */
static void qcom_geni_serial_setup_tx(struct uart_port *uport, u32 xmit_size)
{
	u32 m_cmd;

	writel(xmit_size, uport->membase + SE_UART_TX_TRANS_LEN);
	m_cmd = UART_START_TX << M_OPCODE_SHFT;
	writel(m_cmd, uport->membase + SE_GENI_M_CMD0);
}
/* …（同文件无关代码省略）… */
static void qcom_geni_serial_poll_tx_done(struct uart_port *uport)
{
	int done;

	done = qcom_geni_serial_poll_bit(uport, SE_GENI_M_IRQ_STATUS,
						M_CMD_DONE_EN, true);
	if (!done) {
		writel(M_GENI_CMD_ABORT, uport->membase +
						SE_GENI_M_CMD_CTRL_REG);
		qcom_geni_serial_poll_bit(uport, SE_GENI_M_IRQ_STATUS,
							M_CMD_ABORT_EN, true);
		writel(M_CMD_ABORT_EN, uport->membase + SE_GENI_M_IRQ_CLEAR);
	}
}
/* …（同文件无关代码省略）… */
#endif

#ifdef CONFIG_SERIAL_QCOM_GENI_CONSOLE
static void qcom_geni_serial_drain_fifo(struct uart_port *uport)
{
	struct qcom_geni_serial_port *port = to_dev_port(uport);
/* …（同文件无关代码省略）… */
static void qcom_geni_serial_wr_char(struct uart_port *uport, unsigned char ch)
{
	struct qcom_geni_private_data *private_data = uport->private_data;

	private_data->write_cached_bytes =
		(private_data->write_cached_bytes >> 8) | (ch << 24);
	private_data->write_cached_bytes_cnt++;

	if (private_data->write_cached_bytes_cnt == BYTES_PER_FIFO_WORD) {
		writel(private_data->write_cached_bytes,
		       uport->membase + SE_GENI_TX_FIFOn);
		private_data->write_cached_bytes_cnt = 0;
	}
}
/* …（同文件无关代码省略）… */
static void
__qcom_geni_serial_console_write(struct uart_port *uport, const char *s,
				 unsigned int count, bool poll_rx)
{
	struct qcom_geni_private_data *private_data = uport->private_data;

	int i;
	u32 bytes_to_send = count;

	for (i = 0; i < count; i++) {
		/*
		 * uart_console_write() adds a carriage return for each newline.
		 * Account for additional bytes to be written.
		 */
		if (s[i] == '\n')
			bytes_to_send++;
	}

	writel(DEF_TX_WM, uport->membase + SE_GENI_TX_WATERMARK_REG);
	writel(M_CMD_DONE_EN, uport->membase + SE_GENI_M_IRQ_CLEAR);
	qcom_geni_serial_setup_tx(uport, bytes_to_send);
	for (i = 0; i < count; ) {
		size_t chars_to_write = 0;
		size_t avail = DEF_FIFO_DEPTH_WORDS - DEF_TX_WM;

		/*
		 * If the WM bit never set, then the Tx state machine is not
		 * in a valid state, so break, cancel/abort any existing
		 * command. Unfortunately the current data being written is
		 * lost.
		 */
		if (!qcom_geni_serial_poll_bit(uport, SE_GENI_M_IRQ_STATUS,
						M_TX_FIFO_WATERMARK_EN, true))
			break;
		if (poll_rx)
			qcom_geni_serial_poll_rx_fifo_locked(uport);
		chars_to_write = min_t(size_t, count - i, avail / 2);
		uart_console_write(uport, s + i, chars_to_write,
						qcom_geni_serial_wr_char);
		writel(M_TX_FIFO_WATERMARK_EN, uport->membase +
							SE_GENI_M_IRQ_CLEAR);
		i += chars_to_write;
	}

	if (private_data->write_cached_bytes_cnt) {
		private_data->write_cached_bytes >>= BITS_PER_BYTE *
			(BYTES_PER_FIFO_WORD - private_data->write_cached_bytes_cnt);
		writel(private_data->write_cached_bytes,
		       uport->membase + SE_GENI_TX_FIFOn);
		private_data->write_cached_bytes_cnt = 0;
	}

	qcom_geni_serial_poll_tx_done(uport);
}
/* …（同文件无关代码省略）… */
static void qcom_geni_serial_console_device_unlock(struct console *co,
						    unsigned long flags)
{
	struct qcom_geni_serial_port *port;
#ifdef CONFIG_MAGIC_SYSRQ_SERIAL
	u8 sysrq_ch;
#endif

	port = get_port_from_line(co->index, true, NULL);
	if (IS_ERR(port))
		return;

#ifdef CONFIG_MAGIC_SYSRQ_SERIAL
	/* The threaded console writer can receive a SysRq character. */
	sysrq_ch = port->uport.sysrq_ch;
	port->uport.sysrq_ch = 0;
#endif
	__uart_port_unlock_irqrestore(&port->uport, flags);

#ifdef CONFIG_MAGIC_SYSRQ_SERIAL
	if (sysrq_ch)
		handle_sysrq(sysrq_ch);
#endif
}

static void handle_rx_console(struct uart_port *uport, u32 bytes, bool drop)
{
	u32 i;
	unsigned char buf[sizeof(u32)];
	struct tty_port *tport;
	struct qcom_geni_serial_port *port = to_dev_port(uport);

	tport = &uport->state->port;
	for (i = 0; i < bytes; ) {
		int c;
		int chunk = min_t(int, bytes - i, BYTES_PER_FIFO_WORD);

		ioread32_rep(uport->membase + SE_GENI_RX_FIFOn, buf, 1);
		i += chunk;
		if (drop)
			continue;

		for (c = 0; c < chunk; c++) {
			int sysrq;

			uport->icount.rx++;
			if (port->brk && buf[c] == 0) {
				port->brk = false;
				if (uart_handle_break(uport))
					continue;
			}

			sysrq = uart_prepare_sysrq_char(uport, buf[c]);

			if (!sysrq)
				tty_insert_flip_char(tport, buf[c], TTY_NORMAL);
		}
	}
	if (!drop)
		tty_flip_buffer_push(tport);
}
#else
static void handle_rx_console(struct uart_port *uport, u32 bytes, bool drop)
{

}
#endif /* CONFIG_SERIAL_QCOM_GENI_CONSOLE */

static void handle_rx_uart(struct uart_port *uport, u32 bytes)
{
	struct qcom_geni_serial_port *port = to_dev_port(uport);
	struct tty_port *tport = &uport->state->port;
	int ret;

	ret = tty_insert_flip_string(tport, port->rx_buf, bytes);
	if (ret != bytes) {
		dev_err_ratelimited(uport->dev, "failed to push data (%d < %u)\n",
				ret, bytes);
	}
	uport->icount.rx += ret;
	tty_flip_buffer_push(tport);
}
/* …（同文件无关代码省略）… */
static unsigned int qcom_geni_serial_tx_empty(struct uart_port *uport)
{
	return !readl(uport->membase + SE_GENI_TX_FIFO_STATUS);
}
/* …（同文件无关代码省略）… */
static void qcom_geni_serial_start_tx_fifo(struct uart_port *uport)
{
	unsigned char c;
	u32 irq_en;

	/*
	 * Start a new transfer in case the previous command was cancelled and
	 * left data in the FIFO which may prevent the watermark interrupt
	 * from triggering. Note that the stale data is discarded.
	 */
	if (!qcom_geni_serial_main_active(uport) &&
	    !qcom_geni_serial_tx_empty(uport)) {
		if (uart_fifo_out(uport, &c, 1) == 1) {
			writel(M_CMD_DONE_EN, uport->membase + SE_GENI_M_IRQ_CLEAR);
			qcom_geni_serial_setup_tx(uport, 1);
			writel(c, uport->membase + SE_GENI_TX_FIFOn);
		}
	}

	irq_en = readl(uport->membase +	SE_GENI_M_IRQ_EN);
	irq_en |= M_TX_FIFO_WATERMARK_EN | M_CMD_DONE_EN;
	writel(DEF_TX_WM, uport->membase + SE_GENI_TX_WATERMARK_REG);
	writel(irq_en, uport->membase +	SE_GENI_M_IRQ_EN);
}
/* …（同文件无关代码省略）… */
static void qcom_geni_serial_resume_tx(struct uart_port *uport)
{
	if (!uart_tx_stopped(uport) &&
	    !kfifo_is_empty(&uport->state->port.xmit_fifo))
		qcom_geni_serial_start_tx_fifo(uport);
}
/* …（同文件无关代码省略）… */
	port->tx_queued = 0;
}

static void qcom_geni_serial_handle_rx_fifo(struct uart_port *uport, bool drop)
{
	u32 status;
	u32 word_cnt;
	u32 last_word_byte_cnt;
	u32 last_word_partial;
	u32 total_bytes;

	status = readl(uport->membase +	SE_GENI_RX_FIFO_STATUS);
	word_cnt = status & RX_FIFO_WC_MSK;
	last_word_partial = status & RX_LAST;
	last_word_byte_cnt = (status & RX_LAST_BYTE_VALID_MSK) >>
						RX_LAST_BYTE_VALID_SHFT;

	if (!word_cnt)
		return;
	total_bytes = BYTES_PER_FIFO_WORD * (word_cnt - 1);
	if (last_word_partial && last_word_byte_cnt)
		total_bytes += last_word_byte_cnt;
	else
		total_bytes += BYTES_PER_FIFO_WORD;
	handle_rx_console(uport, total_bytes, drop);
}

/* Caller holds the UART port lock. */
static void qcom_geni_serial_poll_rx_fifo_locked(struct uart_port *uport)
{
	struct qcom_geni_serial_port *port = to_dev_port(uport);
	struct tty_port *tport = &uport->state->port;
	u32 s_irq_status;
	bool drop_rx = false;

	s_irq_status = readl(uport->membase + SE_GENI_S_IRQ_STATUS);
	writel(s_irq_status, uport->membase + SE_GENI_S_IRQ_CLEAR);

	if (s_irq_status & S_RX_FIFO_WR_ERR_EN) {
		uport->icount.overrun++;
		tty_insert_flip_char(tport, 0, TTY_OVERRUN);
	}

	if (s_irq_status & (S_GP_IRQ_0_EN | S_GP_IRQ_1_EN)) {
		if (s_irq_status & S_GP_IRQ_0_EN)
			uport->icount.parity++;
		drop_rx = true;
	} else if (s_irq_status & (S_GP_IRQ_2_EN | S_GP_IRQ_3_EN)) {
		uport->icount.brk++;
		port->brk = true;
	}

	qcom_geni_serial_handle_rx_fifo(uport, drop_rx);
}

static void qcom_geni_serial_stop_rx_fifo(struct uart_port *uport)
{
	u32 irq_en;
/* …（同文件无关代码省略）… */
static void qcom_geni_serial_stop_rx(struct uart_port *uport)
{
	uport->ops->stop_rx(uport);
}
/* …（同文件无关代码省略）… */
static int setup_fifos(struct qcom_geni_serial_port *port)
{
	struct uart_port *uport;
	u32 old_rx_fifo_depth = port->rx_fifo_depth;

	uport = &port->uport;
	port->tx_fifo_depth = geni_se_get_tx_fifo_depth(&port->se);
	port->tx_fifo_width = geni_se_get_tx_fifo_width(&port->se);
	port->rx_fifo_depth = geni_se_get_rx_fifo_depth(&port->se);
	uport->fifosize =
		(port->tx_fifo_depth * port->tx_fifo_width) / BITS_PER_BYTE;

	if (port->rx_buf && (old_rx_fifo_depth != port->rx_fifo_depth) && port->rx_fifo_depth) {
		/*
		 * Use krealloc rather than krealloc_array because rx_buf is
		 * accessed as 1 byte entries as well as 4 byte entries so it's
		 * not necessarily an array.
		 */
		port->rx_buf = devm_krealloc(uport->dev, port->rx_buf,
					     port->rx_fifo_depth * sizeof(u32),
					     GFP_KERNEL);
		if (!port->rx_buf)
			return -ENOMEM;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int qcom_geni_serial_port_setup(struct uart_port *uport)
{
	struct qcom_geni_serial_port *port = to_dev_port(uport);
	u32 rxstale = DEFAULT_BITS_PER_CHAR * STALE_TIMEOUT;
	u32 proto;
	u32 pin_swap;
	int ret;

	proto = geni_se_read_proto(&port->se);
	if (proto == GENI_SE_INVALID_PROTO) {
		ret = geni_load_se_firmware(&port->se, GENI_SE_UART);
		if (ret) {
			dev_err(uport->dev, "UART firmware load failed ret: %d\n", ret);
			return ret;
		}
	} else if (proto != GENI_SE_UART) {
		dev_err(uport->dev, "Invalid FW loaded, proto: %d\n", proto);
		return -ENXIO;
	}

	qcom_geni_serial_stop_rx(uport);

	ret = setup_fifos(port);
	if (ret)
		return ret;

	writel(rxstale, uport->membase + SE_UART_RX_STALE_CNT);

	pin_swap = readl(uport->membase + SE_UART_IO_MACRO_CTRL);
	if (port->rx_tx_swap) {
		pin_swap &= ~DEFAULT_IO_MACRO_IO2_IO3_MASK;
		pin_swap |= IO_MACRO_IO2_IO3_SWAP;
	}
	if (port->cts_rts_swap) {
		pin_swap &= ~DEFAULT_IO_MACRO_IO0_IO1_MASK;
		pin_swap |= IO_MACRO_IO0_SEL;
	}
	/* Configure this register if RX-TX, CTS-RTS pins are swapped */
	if (port->rx_tx_swap || port->cts_rts_swap)
		writel(pin_swap, uport->membase + SE_UART_IO_MACRO_CTRL);

	/*
	 * Make an unconditional cancel on the main sequencer to reset
	 * it else we could end up in data loss scenarios.
	 */
	if (uart_console(uport))
		qcom_geni_serial_poll_tx_done(uport);
	geni_se_config_packing(&port->se, BITS_PER_BYTE, BYTES_PER_FIFO_WORD,
			       false, true, true);
	geni_se_init(&port->se, UART_RX_WM, port->rx_fifo_depth - 2);
	geni_se_select_mode(&port->se, port->dev_data->mode);
	port->setup = true;

	return 0;
}
/* …（同文件无关代码省略）… */
	 * Bump up BW vote on CPU and CORE path as driver supports FIFO mode
	 * only.
	 */
	avg_bw_core = (baud > 115200) ? Bps_to_icc(CORE_2X_50_MHZ)
						: GENI_DEFAULT_BW;
	port->se.icc_paths[GENI_TO_CORE].avg_bw = avg_bw_core;
	port->se.icc_paths[CPU_TO_GENI].avg_bw = Bps_to_icc(baud);
	geni_icc_set_bw(&port->se);

/* …（同文件无关代码省略）… */
static int qcom_geni_console_setup(struct console *co, char *options)
{
	struct uart_port *uport;
	struct qcom_geni_serial_port *port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
	int ret;

	if (co->index >= GENI_UART_CONS_PORTS  || co->index < 0)
		return -ENXIO;

	port = get_port_from_line(co->index, true, NULL);
	if (IS_ERR(port)) {
		pr_err("Invalid line %d\n", co->index);
		return PTR_ERR(port);
	}

	uport = &port->uport;

	if (unlikely(!uport->membase))
		return -ENXIO;

	if (!port->setup) {
		ret = qcom_geni_serial_port_setup(uport);
		if (ret)
			return ret;
	}

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(uport, co, baud, parity, bits, flow);
}

static void qcom_geni_serial_earlycon_write(struct console *con,
					const char *s, unsigned int n)
{
	struct earlycon_device *dev = con->data;

	__qcom_geni_serial_console_write(&dev->port, s, n, false);
}
/* …（同文件无关代码省略）… */
	.device_unlock = qcom_geni_serial_console_device_unlock,
	.device = uart_console_device,
	.setup = qcom_geni_console_setup,
	.flags = CON_PRINTBUFFER | CON_NBCON,
	.index = -1,
	.data = &qcom_geni_console_driver,
/* …（同文件无关代码省略）… */
	struct uart_port *uport = &port->uport;
	struct qcom_geni_private_data *private_data = uport->private_data;

	ret = pm_runtime_force_resume(dev);
	if (ret)
		return ret;

	ret = uart_resume_port(private_data->drv, uport);
	if (uart_console(uport)) {
