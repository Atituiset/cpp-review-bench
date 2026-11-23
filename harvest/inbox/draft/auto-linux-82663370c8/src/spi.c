// AUTO-DRAFT from torvalds/linux PR #fdcd45bc9cfee488b92efc4b7f43d6865ec82770
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void spi_res_release(struct spi_controller *ctlr, struct spi_message *message)
{
	struct spi_res *res, *tmp;

	list_for_each_entry_safe_reverse(res, tmp, &message->resources, entry) {
		if (res->release)
			res->release(ctlr, message, res->data);

		list_del(&res->entry);

		kfree(res);
	}
}
/* …（同文件无关代码省略）… */
static void spi_unmap_buf_attrs(struct spi_controller *ctlr,
				struct device *dev, struct sg_table *sgt,
				enum dma_data_direction dir,
				unsigned long attrs)
{
	dma_unmap_sgtable(dev, sgt, dir, attrs);
	sg_free_table(sgt);
	sgt->orig_nents = 0;
	sgt->nents = 0;
}
/* …（同文件无关代码省略）… */
VISIBLE_IF_KUNIT
int __spi_unmap_msg(struct spi_controller *ctlr, struct spi_message *msg)
{
	struct device *rx_dev = ctlr->cur_rx_dma_dev;
	struct device *tx_dev = ctlr->cur_tx_dma_dev;
	struct spi_transfer *xfer;

	list_for_each_entry(xfer, &msg->transfers, transfer_list) {
		/* The sync has already been done after each transfer. */
		unsigned long attrs = DMA_ATTR_SKIP_CPU_SYNC;

		if (xfer->rx_sg_mapped)
			spi_unmap_buf_attrs(ctlr, rx_dev, &xfer->rx_sg,
					    DMA_FROM_DEVICE, attrs);
		xfer->rx_sg_mapped = false;

		if (xfer->tx_sg_mapped)
			spi_unmap_buf_attrs(ctlr, tx_dev, &xfer->tx_sg,
					    DMA_TO_DEVICE, attrs);
		xfer->tx_sg_mapped = false;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static inline int spi_unmap_msg(struct spi_controller *ctlr,
				struct spi_message *msg)
{
	struct spi_transfer *xfer;
	int ret;

	list_for_each_entry(xfer, &msg->transfers, transfer_list) {
		/*
		 * Restore the original value of tx_buf or rx_buf if they are
		 * NULL.
		 */
		if (xfer->tx_buf == ctlr->dummy_tx)
			xfer->tx_buf = NULL;
		if (xfer->rx_buf == ctlr->dummy_rx)
			xfer->rx_buf = NULL;
	}

	ret = __spi_unmap_msg(ctlr, msg);

	ctlr->cur_rx_dma_dev = NULL;
	ctlr->cur_tx_dma_dev = NULL;

	return ret;
}
/* …（同文件无关代码省略）… */
	if (!was_busy && ctlr->auto_runtime_pm) {
		ret = pm_runtime_get_sync(ctlr->dev.parent);
		if (ret < 0) {
			pm_runtime_put_noidle(ctlr->dev.parent);
			dev_err(&ctlr->dev, "Failed to power device: %d\n",
				ret);

/* …（同文件无关代码省略）… */
				"failed to prepare transfer hardware: %d\n",
				ret);

			if (ctlr->auto_runtime_pm)
				pm_runtime_put(ctlr->dev.parent);

			msg->status = ret;
			spi_finalize_current_message(ctlr);

/* …（同文件无关代码省略）… */
static void __spi_unoptimize_message(struct spi_message *msg)
{
	struct spi_controller *ctlr = msg->spi->controller;

	if (ctlr->unoptimize_message)
		ctlr->unoptimize_message(msg);

	spi_res_release(ctlr, msg);

	msg->optimized = false;
	msg->opt_state = NULL;
}
/* …（同文件无关代码省略）… */
static void spi_maybe_unoptimize_message(struct spi_message *msg)
{
	if (!msg->pre_optimized && msg->optimized &&
	    !msg->spi->controller->defer_optimize_message)
		__spi_unoptimize_message(msg);
}
/* …（同文件无关代码省略）… */
void spi_finalize_current_message(struct spi_controller *ctlr)
{
	struct spi_transfer *xfer;
	struct spi_message *mesg;
	int ret;

	mesg = ctlr->cur_msg;

	if (!ctlr->ptp_sts_supported && !ctlr->transfer_one) {
		list_for_each_entry(xfer, &mesg->transfers, transfer_list) {
			ptp_read_system_postts(xfer->ptp_sts);
			xfer->ptp_sts_word_post = xfer->len;
		}
	}

	if (unlikely(ctlr->ptp_sts_supported))
		list_for_each_entry(xfer, &mesg->transfers, transfer_list)
			WARN_ON_ONCE(xfer->ptp_sts && !xfer->timestamped);

	spi_unmap_msg(ctlr, mesg);

	if (mesg->prepared && ctlr->unprepare_message) {
		ret = ctlr->unprepare_message(ctlr, mesg);
		if (ret) {
			dev_err(&ctlr->dev, "failed to unprepare message: %d\n",
				ret);
		}
	}

	mesg->prepared = false;

	spi_maybe_unoptimize_message(mesg);

	WRITE_ONCE(ctlr->cur_msg_incomplete, false);
	smp_mb(); /* See __spi_pump_transfer_message()... */
	if (READ_ONCE(ctlr->cur_msg_need_completion))
		complete(&ctlr->cur_msg_completion);

	trace_spi_message_done(mesg);

	mesg->state = NULL;
	if (mesg->complete)
		mesg->complete(mesg->context);
}
