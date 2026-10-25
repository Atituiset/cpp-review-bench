// AUTO-DRAFT from torvalds/linux PR #cb8fef4b07f71da01843457b66e579ad248a625e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
int tty_port_alloc_xmit_buf(struct tty_port *port)
{
	/* We may sleep in get_zeroed_page() */
	guard(mutex)(&port->buf_mutex);

	if (port->xmit_buf)
		return 0;

	port->xmit_buf = (u8 *)get_zeroed_page(GFP_KERNEL);
	if (port->xmit_buf == NULL)
		return -ENOMEM;

void tty_port_free_xmit_buf(struct tty_port *port)
{
	guard(mutex)(&port->buf_mutex);
	free_page((unsigned long)port->xmit_buf);
	port->xmit_buf = NULL;
	INIT_KFIFO(port->xmit_fifo);
}
	/* check if last port ref was dropped before tty release */
	if (WARN_ON(port->itty))
		return;
	free_page((unsigned long)port->xmit_buf);
	tty_port_destroy(port);
	if (port->ops && port->ops->destruct)
		port->ops->destruct(port);
