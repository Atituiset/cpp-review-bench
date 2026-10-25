// AUTO-DRAFT from torvalds/linux PR #44388c8f3e22b578c9089df53301ef7e53e7fb01
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define MAX_RETRIES 3
/* …（同文件无关代码省略）… */
#define CFI_SR_DRB		BIT(7)
#define CFI_SR_ESB		BIT(5)
#define CFI_SR_PSB		BIT(4)
#define CFI_SR_WBASB		BIT(3)
#define CFI_SR_SLSB		BIT(1)
/* …（同文件无关代码省略）… */
static int cfi_use_status_reg(struct cfi_private *cfi)
{
	struct cfi_pri_amdstd *extp = cfi->cmdset_priv;
	u8 poll_mask = CFI_POLL_STATUS_REG | CFI_POLL_DQ;

	return extp && extp->MinorVersion >= '5' &&
		(extp->SoftwareFeatures & poll_mask) == CFI_POLL_STATUS_REG;
}
/* …（同文件无关代码省略）… */
static int cfi_check_err_status(struct map_info *map, struct flchip *chip,
				unsigned long adr)
{
	struct cfi_private *cfi = map->fldrv_priv;
	map_word status;

	if (!cfi_use_status_reg(cfi))
		return 0;

	cfi_send_gen_cmd(0x70, cfi->addr_unlock1, chip->start, map, cfi,
			 cfi->device_type, NULL);
	status = map_read(map, adr);

	/* The error bits are invalid while the chip's busy */
	if (!map_word_bitsset(map, status, CMD(CFI_SR_DRB)))
		return 0;

	if (map_word_bitsset(map, status, CMD(0x3a))) {
		unsigned long chipstatus = MERGESTATUS(status);

		if (chipstatus & CFI_SR_ESB)
			pr_err("%s erase operation failed, status %lx\n",
			       map->name, chipstatus);
		if (chipstatus & CFI_SR_PSB)
			pr_err("%s program operation failed, status %lx\n",
			       map->name, chipstatus);
		if (chipstatus & CFI_SR_WBASB)
			pr_err("%s buffer program command aborted, status %lx\n",
			       map->name, chipstatus);
		if (chipstatus & CFI_SR_SLSB)
			pr_err("%s sector write protected, status %lx\n",
			       map->name, chipstatus);

		/* Erase/Program status bits are set on the operation failure */
		if (chipstatus & (CFI_SR_ESB | CFI_SR_PSB))
			return 1;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
		pr_debug("Using buffer write method\n");
		mtd->_write = cfi_amdstd_write_buffers;
	}
}
#endif /* !FORCE_WORD_WRITE */

/* …（同文件无关代码省略）… */
static int is_m29ew(struct cfi_private *cfi)
{
	if (cfi->mfr == CFI_MFR_INTEL &&
	    ((cfi->device_type == CFI_DEVICETYPE_X8 && (cfi->id & 0xff) == 0x7e) ||
	     (cfi->device_type == CFI_DEVICETYPE_X16 && cfi->id == 0x227e)))
		return 1;
	return 0;
}
/* …（同文件无关代码省略）… */
static void cfi_fixup_m29ew_erase_suspend(struct map_info *map,
					  unsigned long adr)
{
	struct cfi_private *cfi = map->fldrv_priv;
	/* before resume, insert a dummy 0xF0 cycle for Micron M29EW devices */
	if (is_m29ew(cfi))
		map_write(map, CMD(0xF0), adr);
}
/* …（同文件无关代码省略）… */
static void cfi_fixup_m29ew_delay_after_resume(struct cfi_private *cfi)
{
	/*
	 * Resolving the Delay After Resume Issue see Micron TN-13-07
	 * Worst case delay must be 500µs but 30-50µs should be ok as well
	 */
	if (is_m29ew(cfi))
		cfi_udelay(500);
}
/* …（同文件无关代码省略）… */
static int __xipram chip_ready(struct map_info *map, struct flchip *chip,
			       unsigned long addr, map_word *expected)
{
	struct cfi_private *cfi = map->fldrv_priv;
	map_word oldd, curd;
	int ret;

	if (cfi_use_status_reg(cfi)) {
		map_word ready = CMD(CFI_SR_DRB);
		/*
		 * For chips that support status register, check device
		 * ready bit
		 */
		cfi_send_gen_cmd(0x70, cfi->addr_unlock1, chip->start, map, cfi,
				 cfi->device_type, NULL);
		curd = map_read(map, addr);

		return map_word_andequal(map, curd, ready, ready);
	}

	oldd = map_read(map, addr);
	curd = map_read(map, addr);

	ret = map_word_equal(map, oldd, curd);

	if (!ret || !expected)
		return ret;

	return map_word_equal(map, curd, *expected);
}
/* …（同文件无关代码省略）… */
static int __xipram chip_good(struct map_info *map, struct flchip *chip,
			      unsigned long addr, map_word *expected)
{
	struct cfi_private *cfi = map->fldrv_priv;
	map_word *datum = expected;

	if (cfi->quirks & CFI_QUIRK_DQ_TRUE_DATA)
		datum = NULL;

	return chip_ready(map, chip, addr, datum);
}
/* …（同文件无关代码省略）… */
static int get_chip(struct map_info *map, struct flchip *chip, unsigned long adr, int mode)
{
	DECLARE_WAITQUEUE(wait, current);
	struct cfi_private *cfi = map->fldrv_priv;
	unsigned long timeo;
	struct cfi_pri_amdstd *cfip = (struct cfi_pri_amdstd *)cfi->cmdset_priv;

 resettime:
	timeo = jiffies + HZ;
 retry:
	switch (chip->state) {

	case FL_STATUS:
		for (;;) {
			if (chip_ready(map, chip, adr, NULL))
				break;

			if (time_after(jiffies, timeo)) {
				printk(KERN_ERR "Waiting for chip to be ready timed out.\n");
				return -EIO;
			}
			mutex_unlock(&chip->mutex);
			cfi_udelay(1);
			mutex_lock(&chip->mutex);
			/* Someone else might have been playing with it. */
			goto retry;
		}
		return 0;

	case FL_READY:
	case FL_CFI_QUERY:
	case FL_JEDEC_QUERY:
		return 0;

	case FL_ERASING:
		if (!cfip || !(cfip->EraseSuspend & (0x1|0x2)) ||
		    !(mode == FL_READY || mode == FL_POINT ||
		    (mode == FL_WRITING && (cfip->EraseSuspend & 0x2))))
			goto sleep;

		/* Do not allow suspend iff read/write to EB address */
		if ((adr & chip->in_progress_block_mask) ==
		    chip->in_progress_block_addr)
			goto sleep;

		/* Erase suspend */
		/* It's harmless to issue the Erase-Suspend and Erase-Resume
		 * commands when the erase algorithm isn't in progress. */
		map_write(map, CMD(0xB0), chip->in_progress_block_addr);
		chip->oldstate = FL_ERASING;
		chip->state = FL_ERASE_SUSPENDING;
		chip->erase_suspended = 1;
		for (;;) {
			if (chip_ready(map, chip, adr, NULL))
				break;

			if (time_after(jiffies, timeo)) {
				/* Should have suspended the erase by now.
				 * Send an Erase-Resume command as either
				 * there was an error (so leave the erase
				 * routine to recover from it) or we trying to
				 * use the erase-in-progress sector. */
				put_chip(map, chip, adr);
				printk(KERN_ERR "MTD %s(): chip not ready after erase suspend\n", __func__);
				return -EIO;
			}

			mutex_unlock(&chip->mutex);
			cfi_udelay(1);
			mutex_lock(&chip->mutex);
			/* Nobody will touch it while it's in state FL_ERASE_SUSPENDING.
			   So we can just loop here. */
		}
		chip->state = FL_READY;
		return 0;

	case FL_XIP_WHILE_ERASING:
		if (mode != F
