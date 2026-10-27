// AUTO-DRAFT from torvalds/linux PR #d135ab453a7400a34930bdda1ef4bdc8acfac80e
	unsigned char *addr = spi->buffer_addr;
	unsigned char *data = spi->buffer_data;

	/* spi message consits of:
	 * first byte: ID and operation
	 */
