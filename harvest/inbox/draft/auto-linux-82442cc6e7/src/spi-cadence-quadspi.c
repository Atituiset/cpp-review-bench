// AUTO-DRAFT from torvalds/linux PR #f47959816751f02a711aab27cf1c805bd4964819
#define CQSPI_REG_WR_COMPLETION_CTRL		0x38
#define CQSPI_REG_WR_DISABLE_AUTO_POLL		BIT(14)
/* …（同文件无关代码省略）… */
	 * care of polling the status register.
	 */
	if (cqspi->wr_completion) {
		reg = readl(reg_base + CQSPI_REG_WR_COMPLETION_CTRL);  // <<< BUG ANCHOR
		reg |= CQSPI_REG_WR_DISABLE_AUTO_POLL;
		writel(reg, reg_base + CQSPI_REG_WR_COMPLETION_CTRL);
		/*
		 * DAC mode require auto polling as flash needs to be polled
