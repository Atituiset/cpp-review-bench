// AUTO-DRAFT from torvalds/linux PR #22430ae5d90ab288b0ee2ad99ae941f4a666b694
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static bool wait_for_lsr(struct uart_8250_port *up, int bits)
{
	unsigned int status, tmout;

	/*
	 * Wait for a character to be sent. Fallback to a safe default
	 * timeout value if @frame_time is not available.
	 */
	if (up->port.frame_time)
		tmout = up->port.frame_time * 2 / NSEC_PER_USEC;
	else
		tmout = 10000;

	for (;;) {
		status = serial_lsr_in(up);

		if ((status & bits) == bits)
			break;
		if (--tmout == 0)
			break;
		udelay(1);
		touch_nmi_watchdog();
	}

	return (tmout != 0);
}
/* …（同文件无关代码省略）… */
void serial8250_fifo_wait_for_lsr_thre(struct uart_8250_port *up,
				       struct nbcon_write_context *wctxt,
				       unsigned int count)
{
	unsigned int i;

	/*
	 * For console writing, enter/exit an unsafe section for each byte
	 * in order to pass the ownership as quickly as possible if a higher
	 * priority context wants ownership. Otherwise, an attempt to take
	 * over the ownership might timeout. The new owner will wait for
	 * UART_LSR_THRE before reusing the fifo.
	 */
	for (i = 0; i < count; i++) {
		bool tx_ready;

		if (wctxt && !nbcon_enter_unsafe(wctxt))
			return;

		tx_ready = wait_for_lsr(up, UART_LSR_THRE);

		if (wctxt)
			nbcon_exit_unsafe(wctxt);

		if (tx_ready)
			break;
	}
}
/* …（同文件无关代码省略）… */
static void serial8250_console_putchar(struct uart_port *port, unsigned char ch)
{
	struct uart_8250_port *up = up_to_u8250p(port);

	serial_port_out(port, UART_TX, ch);

	up->console_line_ended = (ch == '\n');
}
/* …（同文件无关代码省略）… */
static void __serial8250_console_fifo_write(struct uart_8250_port *up,
					    struct nbcon_write_context *wctxt,
					    const char *s, unsigned int count)
{
	const char *end = s + count;
	unsigned int fifosize = up->tx_loadsz;
	struct uart_port *port = &up->port;
	unsigned int tx_count = 0;
	bool cr_sent = false;
	unsigned int i;

	while (s != end) {
		/* Allow timeout for each byte of a possibly full FIFO */
		serial8250_fifo_wait_for_lsr_thre(up, wctxt, fifosize);

		/*
		 * Fill the FIFO. If a handover or takeover occurs, writing
		 * must be aborted since the string data is no longer valid.
		 */
		for (i = 0; i < fifosize && s != end; ++i) {
			if (!nbcon_enter_unsafe(wctxt))
				return;

			if (*s == '\n' && !cr_sent) {
				serial8250_console_putchar(port, '\r');
				cr_sent = true;
			} else {
				serial8250_console_putchar(port, *s++);
				cr_sent = false;
			}

			nbcon_exit_unsafe(wctxt);
		}
		tx_count = i;
	}

	/*
	 * Allow timeout for each byte written since the caller will only wait
	 * for UART_LSR_BOTH_EMPTY using the timeout of a single character
	 */
	serial8250_fifo_wait_for_lsr_thre(up, wctxt, tx_count);
}
/* …（同文件无关代码省略）… */
	 * If the console printer did not fully output the previous line, it
	 * must have been handed or taken over. Insert a newline in order to
	 * maintain clean output.
	 */
	if (!up->console_line_ended) {
		if (use_fifo)
			__serial8250_console_fifo_write(up, wctxt, "\n", 1);
		else
