// AUTO-DRAFT from torvalds/linux PR #b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
struct hp300_port {
	struct hp300_port *next;	/* next port */
	struct uart_8250_port *uport;
};
/* …（同文件无关代码省略）… */
static struct hp300_port *hp300_ports;
#endif

#ifdef CONFIG_HPDCA

static int hpdca_init_one(struct dio_dev *d,
					const struct dio_device_id *ent);
static void hpdca_remove_one(struct dio_dev *d);

static struct dio_device_id hpdca_dio_tbl[] = {
	{ DIO_ID_DCA0 },
	{ DIO_ID_DCA0REM },
	{ DIO_ID_DCA1 },
	{ DIO_ID_DCA1REM },
	{ 0 }
};

static struct dio_driver hpdca_driver = {
	.name      = "hpdca",
	.id_table  = hpdca_dio_tbl,
	.probe     = hpdca_init_one,
	.remove    = hpdca_remove_one,
};

#endif

static unsigned int num_ports;

extern int hp300_uart_scode;
/* …（同文件无关代码省略）… */
#define UART_OFFSET	17
/* …（同文件无关代码省略）… */
#define DCA_ID		0x01	/* ID (read), reset (write) */
#define DCA_IC		0x03	/* Interrupt control        */
/* …（同文件无关代码省略）… */
#define DCA_IC_IE	0x80	/* Master interrupt enable  */
/* …（同文件无关代码省略）… */
#define HPDCA_BAUD_BASE 153600
/* …（同文件无关代码省略）… */
#define FRODO_BASE	(0x41c000)
/* …（同文件无关代码省略）… */
#define FRODO_APCIBASE		0x0
#define FRODO_APCISPACE		0x20
#define FRODO_APCI_OFFSET(x)	(FRODO_APCIBASE + ((x) * FRODO_APCISPACE))
/* …（同文件无关代码省略）… */
#define HPAPCI_BAUD_BASE 500400
/* …（同文件无关代码省略）… */
static int hpdca_init_one(struct dio_dev *d,
				const struct dio_device_id *ent)
{
	struct uart_8250_port *uport, uart;

#ifdef CONFIG_SERIAL_8250_CONSOLE
	if (hp300_uart_scode == d->scode) {
		/* Already got it. */
		return 0;
	}
#endif
	memset(&uart, 0, sizeof(uart));

	/* Memory mapped I/O */
	uart.port.iotype = UPIO_MEM;
	uart.port.flags = UPF_SKIP_TEST | UPF_SHARE_IRQ | UPF_BOOT_AUTOCONF;
	uart.port.irq = d->ipl;
	uart.port.uartclk = HPDCA_BAUD_BASE * 16;
	uart.port.mapbase = (d->resource.start + UART_OFFSET);
	uart.port.membase = (char *)(uart.port.mapbase + DIO_VIRADDRBASE);
	uart.port.regshift = 1;
	uart.port.dev = &d->dev;
	uport = serial8250_register_8250_port(&uart);
	if (IS_ERR(uport)) {
		dev_notice(&d->dev,
			  "8250_hp300: register_serial() DCA scode %d irq %d failed\n",
			  d->scode, uart.port.irq);
		return -ENOMEM;
	}

	/* Enable board-interrupts */
	out_8(d->resource.start + DIO_VIRADDRBASE + DCA_IC, DCA_IC_IE);
	dio_set_drvdata(d, uport);

	/* Reset the DCA */
	out_8(d->resource.start + DIO_VIRADDRBASE + DCA_ID, 0xff);
	udelay(100);

	num_ports++;

	return 0;
}
#endif

static int __init hp300_8250_init(void)
{
	static int called;
#ifdef CONFIG_HPAPCI
	unsigned long base;
	struct uart_8250_port uart;
	struct hp300_port *port;
	int i;
#endif
	if (called)
		return -ENODEV;
	called = 1;

	if (!MACH_IS_HP300)
		return -ENODEV;

#ifdef CONFIG_HPDCA
	dio_register_driver(&hpdca_driver);
#endif
#ifdef CONFIG_HPAPCI
	if (hp300_model < HP_400) {
		if (!num_ports)
			return -ENODEV;
		return 0;
	}
	/* These models have the Frodo chip.
	 * Port 0 is reserved for the Apollo Domain keyboard.
	 * Port 1 is either the console or the DCA.
	 */
	for (i = 1; i < 4; i++) {
		/* Port 1 is the console on a 425e, on other machines it's
		 * mapped to DCA.
		 */
#ifdef CONFIG_SERIAL_8250_CONSOLE
		if (i == 1)
			continue;
#endif

		/* Create new serial device */
		port = kmalloc_obj(struct hp300_port);
		if (!port)
			return -ENOMEM;

		memset(&uart, 0, sizeof(uart));

		base = (FRODO_BASE + FRODO_APCI_OFFSET(i));

		/* Memory mapped I/O */
		uart.port.iotype = UPIO_MEM;
		uart.port.flags = UPF_SKIP_TEST | UPF_SHARE_IRQ
				| UPF_BOOT_AUTOCONF;
		/* XXX - no interrupt support yet */
		uart.port.irq = 0;
		uart.port.uartclk = HPAPCI_BAUD_BASE * 16;
		uart.port.mapbase = base;
		uart.port.membase = (char *)(base + DIO_VIRADDRBASE);
		uart.port.regshift = 2;

		port->uport = serial8250_register_8250_port(&uart);
		if (IS_ERR(port->uport)) {
			dev_notice(uart.port.dev,
				   "8250_hp300: register_serial() APCI %d irq %d failed\n",
				   i, uart.port.irq);
			kfree(port);
			continue;
		}

		port->next = hp300_ports;
		hp300_ports = port;

		num_ports++;
	}
#endif

	/* Any boards found? */
	if (!num_ports)
		return -ENODEV;

	return 0;
}

#ifdef CONFIG_HPDCA
static void hpdca_remove_one(struct dio_dev *d)
{
	struct uart_8250_port *uport = dio_get_drvdata(d);

	if (d->resource.start) {
		/* Disable board-interrupts */
		out_8(d->resource.start + DIO_VIRADDRBASE + DCA_IC, 0);
	}
	serial8250_unregister_port(uport);
}
#endif

static void __exit hp300_8250_exit(void)
{
#ifdef CONFIG_HPAPCI
