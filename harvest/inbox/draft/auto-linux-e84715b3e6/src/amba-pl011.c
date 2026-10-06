// AUTO-DRAFT from torvalds/linux PR #22430ae5d90ab288b0ee2ad99ae941f4a666b694
static unsigned int pl011_reg_to_offset(const struct uart_amba_port *uap,
					unsigned int reg)
{
	return uap->reg_offset[reg];
}
/* …（同文件无关代码省略）… */
static unsigned int pl011_read(const struct uart_amba_port *uap,
			       unsigned int reg)
{
	void __iomem *addr = uap->port.membase + pl011_reg_to_offset(uap, reg);
  // <<< BUG ANCHOR
	return (uap->port.iotype == UPIO_MEM32) ?
		readl_relaxed(addr) : readw_relaxed(addr);
}
/* …（同文件无关代码省略）… */
static void pl011_write(unsigned int val, const struct uart_amba_port *uap,
			unsigned int reg)
{
	void __iomem *addr = uap->port.membase + pl011_reg_to_offset(uap, reg);

	if (uap->port.iotype == UPIO_MEM32)
		writel_relaxed(val, addr);
	else
		writew_relaxed(val, addr);
}
/* …（同文件无关代码省略）… */
static void pl011_console_putchar(struct uart_port *port, unsigned char ch)
{
	struct uart_amba_port *uap =
	    container_of(port, struct uart_amba_port, port);

	while (pl011_read(uap, REG_FR) & UART01x_FR_TXFF)
		cpu_relax();
	pl011_write(ch, uap, REG_DR);
	uap->console_line_ended = (ch == '\n');
}
/* …（同文件无关代码省略）… */
				uap, REG_CR);
	}

	if (!uap->console_line_ended)
		uart_console_write(&uap->port, "\n", 1, pl011_console_putchar);
	uart_console_write(&uap->port, wctxt->outbuf, wctxt->len, pl011_console_putchar);
