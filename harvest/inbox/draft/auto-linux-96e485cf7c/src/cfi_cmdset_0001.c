// AUTO-DRAFT from torvalds/linux PR #a243ede718463c7b481878656f1ff32a0ce0fd54
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define M28F00AP30	0x8963
/* …（同文件无关代码省略）… */
#define LH28F640BFHE_PTTL90	0x00b0
#define LH28F640BFHE_PBTL90	0x00b1
#define LH28F640BFHE_PTTL70A	0x00b2
#define LH28F640BFHE_PBTL70A	0x00b3
/* …（同文件无关代码省略）… */
static int is_LH28F640BF(struct cfi_private *cfi)
{
	/* Sharp LH28F640BF Family */
	if (cfi->mfr == CFI_MFR_SHARP && (
	    cfi->id == LH28F640BFHE_PTTL90 || cfi->id == LH28F640BFHE_PBTL90 ||
	    cfi->id == LH28F640BFHE_PTTL70A || cfi->id == LH28F640BFHE_PBTL70A))
		return 1;
	return 0;
}
/* …（同文件无关代码省略）… */
static int cfi_is_micron_28F00AP30(struct cfi_private *cfi, struct flchip *chip)
{
	/*
	 * Micron(was Numonyx) 1Gbit bottom boot are buggy w.r.t
	 * Erase Supend for their small Erase Blocks(0x8000)
	 */
	if (cfi->mfr == CFI_MFR_INTEL && cfi->id == M28F00AP30)
		return 1;
	return 0;
}
/* …（同文件无关代码省略）… */
static int chip_ready (struct map_info *map, struct flchip *chip, unsigned long adr, int mode)
{
	DECLARE_WAITQUEUE(wait, current);
	struct cfi_private *cfi = map->fldrv_priv;
	map_word status, status_OK = CMD(0x80), status_PWS = CMD(0x01);
	struct cfi_pri_intelext *cfip = cfi->cmdset_priv;
	unsigned long timeo = jiffies + HZ;

	/* Prevent setting state FL_SYNCING for chip in suspended state. */
	if (mode == FL_SYNCING && chip->oldstate != FL_READY)
		goto sleep;

	switch (chip->state) {

	case FL_STATUS:
		for (;;) {
			status = map_read(map, adr);
			if (map_word_andequal(map, status, status_OK, status_OK))
				break;

			/* At this point we're fine with write operations
			   in other partitions as they don't conflict. */
			if (chip->priv && map_word_andequal(map, status, status_PWS, status_PWS))
				break;

			mutex_unlock(&chip->mutex);
			cfi_udelay(1);
			mutex_lock(&chip->mutex);
			/* Someone else might have been playing with it. */
			return -EAGAIN;
		}
		fallthrough;
	case FL_READY:
	case FL_CFI_QUERY:
	case FL_JEDEC_QUERY:
		return 0;

	case FL_ERASING:
		if (!cfip ||
		    !(cfip->FeatureSupport & 2) ||
		    !(mode == FL_READY || mode == FL_POINT ||
		     (mode == FL_WRITING && (cfip->SuspendCmdSupport & 1))))
			goto sleep;

		/* Do not allow suspend iff read/write to EB address */
		if ((adr & chip->in_progress_block_mask) ==
		    chip->in_progress_block_addr)
			goto sleep;

		/* do not suspend small EBs, buggy Micron Chips */
		if (cfi_is_micron_28F00AP30(cfi, chip) &&
		    (chip->in_progress_block_mask == ~(0x8000-1)))
			goto sleep;

		/* Erase suspend */
		map_write(map, CMD(0xB0), chip->in_progress_block_addr);

		/* If the flash has finished erasing, then 'erase suspend'
		 * appears to make some (28F320) flash devices switch to
		 * 'read' mode.  Make sure that we switch to 'read status'
		 * mode so we get the right data. --rmk
		 */
		map_write(map, CMD(0x70), chip->in_progress_block_addr);
		chip->oldstate = FL_ERASING;
		chip->state = FL_ERASE_SUSPENDING;
		chip->erase_suspended = 1;
		for (;;) {
			status = map_read(map, chip->in_progress_block_addr);
			if (map_word_andequal(map, status, status_OK, status_OK))
			        break;

			if (time_after(jiffies, timeo)) {
				/* Urgh. Resume and pretend we weren't here.
				 * Make sure we're in 'read status' mode if it had finished */
				put_chip(map, chip, adr);
				printk(KERN_ERR "%s: Chip not ready after erase "
				       "suspended: status = 0x%lx\n", map->name, status.x[0]);
				return -EIO;
			}

			mutex_unlock(&chip->mutex);
			cfi_udelay(1);
			mutex_lock(&chip->mutex);
			/* Nobody will touch it while it's in state FL_ERASE_SUSPENDING.
			   So we can just loop here. */
		}
		chip->state = FL_STATUS;
		return 0;

	case FL_XIP_WHILE_ERASING:
		if (mode != FL_READY && mode != FL_POINT &&
		    (mode != FL_WRITING || !cfip || !(cfip->SuspendCmdSupport&1)))
			goto sleep;
		chip->oldstate = chip->state;
		chip->state = FL_READY;
		return 0;

	case FL_SHUTDOWN:
		/* The machine is rebooting now,so no one can get chip anymore */
		return -EIO;
	case FL_POINT:
		/* Only if there's no operation suspended... */
		if (mode == FL_READY && chip->oldstate == FL_READY)
			return 0;
		fallthrough;
	default:
	sleep:
		set_current_state(TASK_UNINTERRUPTIBLE);
		add_wait_queue(&chip->wq, &wait);
		mutex_unlock(&chip->mutex);
		schedule();
		remove_wait_queue(&chip->wq, &wait);
		mutex_lock(&chip->mutex);
		return -EAGAIN;
	}
}
/* …（同文件无关代码省略）… */
static int get_chip(struct map_info *map, struct flchip *chip, unsigned long adr, int mode)
{
	int ret;
	DECLARE_WAITQUEUE(wait, current);

 retry:
	if (chip->priv &&
	    (mode == FL_WRITING || mode == FL_ERASING || mode == FL_OTP_WRITE
	    || mode == FL_SHUTDOWN) && chip->state != FL_SYNCING) {
		/*
		 * OK. We have possibility for contention on the write/erase
		 * operations which are global to the real chip and not per
		 * partition.  So let's fight it over in the partition which
		 * currently has authority on the operation.
		 *
		 * The rules are as follows:
		 *
		 * - any write operation must own shared->writing.
		 *
		 * - any erase operation must own _both_ shared->writing and
		 *   shared->erasing.
		 *
		 * - contention arbitration is handled in the owner's context.
		 *
		 * The 'shared' struct can be read and/or written only when
		 * its lock is taken.
		 */
		struct flchip_shared *shared = chip->priv;
		struct flchip *contender;
		mutex_lock(&shared->lock);
		contender = shared->writing;
		if (contender && contender != chip) {
			/*
			 * The engine to perform desired operation on this
			 * partition is already in use by someone else.
			 * Let's fight over it in the context of the chip
			 * currently using it.  If it is possible to suspend,
			 * that other partition will do just that, otherwise
			 * it'll happily send us to sleep.  In any case, when
			 * get_chip returns success we're clear to go ahead.
			 */
			ret = mutex_trylock(&contender->mutex);
			mutex_unlock(&shared->lock);
			if (!ret)
				goto retry;
			mutex_unlock(&chip->mutex);
			ret = chip_ready(map, contender, contender->start, mode);
			mutex_lock(&chip->mutex);

			if (ret == -EAGAIN) {
				mutex_unlock(&contender->mutex);
				goto retry;
			}
			if (ret) {
				mutex_unlock(&contender->mutex);
				return ret;
			}
			mutex_lock(&shared->lock);

			/* We should not own chip if it is already
			 * in FL_SYNCING state. Put contender and retry. */
			if (chip->state == FL_SYNCING) {
				put_chip(map, contender, contender->start);
				mutex_unlock(&contender->mutex);
				goto retry;
			}
			mutex_unlock(&contender->mutex);
		}

		/* Check if we already have suspended erase
		 * on this chip. Sleep. */
		if (mode == FL_ERASING && shared->erasing
		    && shared->erasing->oldstate == FL_ERASING) {
			mutex_unlock(&shared->lock);
			set_current_state(TASK_UNINTERRUPTIBLE);
			add_wait_queue(&chip->wq, &wait);
			mutex_unlock(&chip->mutex);
			schedule();
			remove_wait_queue(&chip->wq, &wait);
			mutex_lock(&chip->mutex);
			goto retry;
		}

		/* We now own it */
		shared->writing = chip;
		if (mode == FL_ERASING)
			shared->erasing = chip;
		mutex_unlock(&shared->lock);
	}
	ret = chip_ready(map, chip, adr, mode);
	if (ret == -EAGAIN)
		goto retry;

	return ret;
}
/* …（同文件无关代码省略）… */
static void put_chip(struct map_info *map, struct flchip *chip, unsigned long adr)
{
	struct cfi_private *cfi = map->fldrv_priv;

	if (chip->priv) {
		struct flchip_shared *shared = chip->priv;
		mutex_lock(&shared->lock);
		if (shared->writing == chip && chip->oldstate == FL_READY) {
			/* We own the ability to write, but we're done */
			shared->writing = shared->erasing;
			if (shared->writing && shared->writing != chip) {
				/* give back ownership to who we loaned it from */
				struct flchip *loaner = shared->writing;
				mutex_lock(&loaner->mutex);
				mutex_unlock(&shared->lock);
				mutex_unlock(&chip->mutex);
				put_chip(map, loaner, loaner->start);
				mutex_lock(&chip->mutex);
				mutex_unlock(&loaner->mutex);
				wake_up(&chip->wq);
				return;
			}
			shared->erasing = NULL;
			shared->writing = NULL;
		} else if (shared->erasing == chip && shared->writing != chip) {
			/*
			 * We own the ability to erase without the ability
			 * to write, which means the erase was suspended
			 * and some other partition is currently writing.
			 * Don't let the switch below mess things up since
			 * we don't have ownership to resume anything.
			 */
			mutex_unlock(&shared->lock);
			wake_up(&chip->wq);
			return;
		}
		mutex_unlock(&shared->lock);
	}

	switch(chip->oldstate) {
	case FL_ERASING:
		/* What if one interleaved chip has finished and the
		   other hasn't? The old code would leave the finished
		   one in READY mode. That's bad, and caused -EROFS
		   errors to be returned from do_erase_oneblock because
		   that's the only bit it checked for at the time.
		   As the state machine appears to explicitly allow
		   sending the 0x70 (Read Status) command to an erasing
		   chip and expecting it to be ignored, that's what we
		   do. */
		map_write(map, CMD(0xd0), chip->in_progress_block_addr);
		map_write(map, CMD(0x70), chip->in_progress_block_addr);
		chip->oldstate = FL_READY;
		chip->state = FL_ERASING;
		break;

	case FL_XIP_WHILE_ERASING:
		chip->state = chip->oldstate;
		chip->oldstate = FL_READY;
		break;

	case FL_READY:
	case FL_STATUS:
	case FL_JEDEC_QUERY:
		break;
	default:
		printk(KERN_ERR "%s: put_chip() called with oldstate %d!!\n", map->name, chip->oldstate);
	}
	wake_up(&chip->wq);
}
/* …（同文件无关代码省略）… */
static int __xipram xip_wait_for_operation(
		struct map_info *map, struct flchip *chip,
		unsigned long adr, unsigned int chip_op_time_max)
{
	struct cfi_private *cfi = map->fldrv_priv;
	struct cfi_pri_intelext *cfip = cfi->cmdset_priv;
	map_word status, OK = CMD(0x80);
	unsigned long usec, suspended, start, done;
	flstate_t oldstate, newstate;

       	start = xip_currtime();
	usec = chip_op_time_max;
	if (usec == 0)
		usec = 500000;
	done = 0;

	do {
		cpu_relax();
		if (xip_irqpending() && cfip &&
		    ((chip->state == FL_ERASING && (cfip->FeatureSupport&2)) ||
		     (chip->state == FL_WRITING && (cfip->FeatureSupport&4))) &&
		    (cfi_interleave_is_1(cfi) || chip->oldstate == FL_READY)) {
			/*
			 * Let's suspend the erase or write operation when
			 * supported.  Note that we currently don't try to
			 * suspend interleaved chips if there is already
			 * another operation suspended (imagine what happens
			 * when one chip was already done with the current
			 * operation while another chip suspended it, then
			 * we resume the whole thing at once).  Yes, it
			 * can happen!
			 */
			usec -= done;
			map_write(map, CMD(0xb0), adr);
			map_write(map, CMD(0x70), adr);
			suspended = xip_currtime();
			do {
				if (xip_elapsed_since(suspended) > 100000) {
					/*
					 * The chip doesn't want to suspend
					 * after waiting for 100 msecs.
					 * This is a critical error but there
					 * is not much we can do here.
					 */
					return -EIO;
				}
				status = map_read(map, adr);
			} while (!map_word_andequal(map, status, OK, OK));

			/* Suspend succeeded */
			oldstate = chip->state;
			if (oldstate == FL_ERASING) {
				if (!map_word_bitsset(map, status, CMD(0x40)))
					break;
				newstate = FL_XIP_WHILE_ERASING;
				chip->erase_suspended = 1;
			} else {
				if (!map_word_bitsset(map, status, CMD(0x04)))
					break;
				newstate = FL_XIP_WHILE_WRITING;
				chip->write_suspended = 1;
			}
			chip->state = newstate;
			map_write(map, CMD(0xff), adr);
			(void) map_read(map, adr);
			xip_iprefetch();
			local_irq_enable();
			mutex_unlock(&chip->mutex);
			xip_iprefetch();
			cond_resched();

			/*
			 * We're back.  However someone else might have
			 * decided to go write to the chip if we are in
			 * a suspended erase state.  If so let's wait
			 * until it's done.
			 */
			mutex_lock(&chip->mutex);
			while (chip->state != newstate) {
				DECLARE_WAITQUEUE(wait, current);
				set_current_state(TASK_UNINTERRUPTIBLE);
				add_wait_queue(&chip->wq, &wait);
				mutex_unlock(&chip->mutex);
				schedule();
				remove_wait_queue(&chip->wq, &wait);
				mutex_lock(&chip->mutex);
			}
			/* Disallow XIP again */
			local_irq_disable();

			/* Resume the write or erase operation */
			map_write(map, CMD(0xd0), adr);
			map_write(map, CMD(0x70), adr);
			chip->state = oldstate;
			start = xip_currtime();
		} else if (usec >= 1000000/HZ) {
			/*
			 * Try to save on CPU power when waiting delay
			 * is at least a system timer tick period.
			 * No need to be extremely accurate here.
			 */
			xip_cpu_idle();
		}
		status = map_read(map, adr);
		done = xip_elapsed_since(start);
	} while (!map_word_andequal(map, status, OK, OK)
		 && done < usec);

	return (done >= usec) ? -ETIME : 0;
}
/* …（同文件无关代码省略）… */
#define INVAL_CACHE_AND_WAIT(map, chip, cmd_adr, inval_adr, inval_len, usec, usec_max) \
	xip_wait_for_operation(map, chip, cmd_adr, usec_max)
/* …（同文件无关代码省略）… */
#define xip_disable(map, chip, adr)
#define xip_enable(map, chip, adr)
/* …（同文件无关代码省略）… */
#define WAIT_TIMEOUT(map, chip, adr, udelay, udelay_max) \
	INVAL_CACHE_AND_WAIT(map, chip, adr, 0, 0, udelay, udelay_max);
/* …（同文件无关代码省略）… */
}


static int __xipram do_write_buffer(struct map_info *map, struct flchip *chip,
				    unsigned long adr, const struct kvec **pvec,
				    unsigned long *pvec_seek, int len)
{
	struct cfi_private *cfi = map->fldrv_priv;
	map_word status, write_cmd, datum;
	unsigned long cmd_adr;
	int ret, wbufsize, word_gap, words;
	const struct kvec *vec;
/* …（同文件无关代码省略）… */
	if (is_LH28F640BF(cfi))
		cmd_adr = adr;

	/* Let's determine this according to the interleave only once */
	write_cmd = (cfi->cfiq->P_ID != P_ID_INTEL_PERFORMANCE) ? CMD(0xe8) : CMD(0xe9);

	mutex_lock(&chip->mutex);
	ret = get_chip(map, chip, cmd_adr, FL_WRITING);
	if (ret) {
/* …（同文件无关代码省略）… */
	   So we must check here and reset those bits if they're set. Otherwise
	   we're just pissing in the wind */
	if (chip->state != FL_STATUS) {
		map_write(map, CMD(0x70), cmd_adr);
		chip->state = FL_STATUS;
	}
	status = map_read(map, cmd_adr);
	if (map_word_bitsset(map, status, CMD(0x30))) {
		xip_enable(map, chip, cmd_adr);
		printk(KERN_WARNING "SR.4 or SR.5 bits set in buffer write (status %lx). Clearing.\n", status.x[0]);
		xip_disable(map, chip, cmd_adr);
		map_write(map, CMD(0x50), cmd_adr);
		map_write(map, CMD(0x70), cmd_adr);
	}

	chip->state = FL_WRITING_TO_BUFFER;
	map_write(map, write_cmd, cmd_adr);
	ret = WAIT_TIMEOUT(map, chip, cmd_adr, 0, 0);
	if (ret) {
		/* Argh. Not ready for write to buffer */
		map_word Xstatus = map_read(map, cmd_adr);
		map_write(map, CMD(0x70), cmd_adr);
		chip->state = FL_STATUS;
		status = map_read(map, cmd_adr);
		map_write(map, CMD(0x50), cmd_adr);
		map_write(map, CMD(0x70), cmd_adr);
		xip_enable(map, chip, cmd_adr);
		printk(KERN_ERR "%s: Chip not ready for buffer write. Xstatus = %lx, status = %lx\n",
				map->name, Xstatus.x[0], status.x[0]);
/* …（同文件无关代码省略）… */
	}

	/* Write length of data to come */
	map_write(map, CMD(words), cmd_adr );

	/* Write data */
	vec = *pvec;
/* …（同文件无关代码省略）… */
	*pvec_seek = vec_seek;

	/* GO GO GO */
	map_write(map, CMD(0xd0), cmd_adr);
	chip->state = FL_WRITING;

	ret = INVAL_CACHE_AND_WAIT(map, chip, cmd_adr,
				   initial_adr, initial_len,
				   chip->buffer_write_time,
				   chip->buffer_write_time_max);
	if (ret) {
		map_write(map, CMD(0x70), cmd_adr);
		chip->state = FL_STATUS;
		xip_enable(map, chip, cmd_adr);
		printk(KERN_ERR "%s: buffer write error (status timeout)\n", map->name);
/* …（同文件无关代码省略）… */
		unsigned long chipstatus = MERGESTATUS(status);

		/* reset status */
		map_write(map, CMD(0x50), cmd_adr);
		map_write(map, CMD(0x70), cmd_adr);
		xip_enable(map, chip, cmd_adr);

		if (chipstatus & 0x02) {
