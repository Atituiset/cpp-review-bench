// AUTO-DRAFT from torvalds/linux PR #ae3690eb2a994dd74a43639d6bf800b51f18b48c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define MSGDMA_CSR_STATUS		0x00	/* Read / Clear */
/* …（同文件无关代码省略）… */
#define MSGDMA_CSR_RESP_FILL_LEVEL	0x0c	/* response FIFO fill level */
/* …（同文件无关代码省略）… */
#define MSGDMA_CSR_STAT_BUSY			BIT(0)
/* …（同文件无关代码省略）… */
#define MSGDMA_CSR_STAT_DESC_BUF_FULL		BIT(2)
/* …（同文件无关代码省略）… */
#define MSGDMA_CSR_STAT_IRQ			BIT(9)
/* …（同文件无关代码省略）… */
#define MSGDMA_RESP_BYTES_TRANSFERRED	0x00
#define MSGDMA_RESP_STATUS		0x04
/* …（同文件无关代码省略）… */
struct msgdma_device {
	spinlock_t lock;
	struct device *dev;
	struct tasklet_struct irq_tasklet;
	struct list_head pending_list;
	struct list_head free_list;
	struct list_head active_list;
	struct list_head done_list;
	u32 desc_free_cnt;
	bool idle;

	struct dma_device dmadev;
	struct dma_chan	dmachan;
	dma_addr_t hw_desq;
	struct msgdma_sw_desc *sw_desq;
	unsigned int npendings;

	struct dma_slave_config slave_cfg;

	int irq;

	/* mSGDMA controller */
	void __iomem *csr;

	/* mSGDMA descriptors */
	void __iomem *desc;

	/* mSGDMA response */
	void __iomem *resp;
};
/* …（同文件无关代码省略）… */
static void msgdma_free_descriptor(struct msgdma_device *mdev,
				   struct msgdma_sw_desc *desc)
{
	struct msgdma_sw_desc *child, *next;

	mdev->desc_free_cnt++;
	list_move_tail(&desc->node, &mdev->free_list);
	list_for_each_entry_safe(child, next, &desc->tx_list, node) {
		mdev->desc_free_cnt++;
		list_move_tail(&child->node, &mdev->free_list);
	}
}
/* …（同文件无关代码省略）… */
static void msgdma_copy_one(struct msgdma_device *mdev,
			    struct msgdma_sw_desc *desc)
{
	void __iomem *hw_desc = mdev->desc;

	/* Ensure control is the last field — required for correct FIFO flush ordering */
	static_assert(offsetof(struct msgdma_extended_desc, control) ==
		      sizeof(struct msgdma_extended_desc) - sizeof(u32),
		      "control must be the last field in msgdma_extended_desc");

	/*
	 * Check if the DESC FIFO it not full. If its full, we need to wait
	 * for at least one entry to become free again
	 */
	while (ioread32(mdev->csr + MSGDMA_CSR_STATUS) &
	       MSGDMA_CSR_STAT_DESC_BUF_FULL)
		mdelay(1);

	/* Ensure control is the last field — required for correct FIFO flush ordering */
	static_assert(offsetof(struct msgdma_extended_desc, control) ==
			sizeof(struct msgdma_extended_desc) - sizeof(u32),
			"control must be the last field in msgdma_extended_desc");

	/*
	 * Copy the descriptor into the descriptor FIFO of the DMA controller,
	 * excluding the control word. The FIFO is flushed and the descriptor
	 * becomes valid once the control word is written last.
	 */
	memcpy_toio(hw_desc, &desc->hw_desc,
		    offsetof(struct msgdma_extended_desc, control));

	/* Write control word last to flush this descriptor into the FIFO */
	mdev->idle = false;
	wmb();
	iowrite32(desc->hw_desc.control, hw_desc +
		  offsetof(struct msgdma_extended_desc, control));
	wmb();
}
/* …（同文件无关代码省略）… */
static void msgdma_copy_desc_to_fifo(struct msgdma_device *mdev,
				     struct msgdma_sw_desc *desc)
{
	struct msgdma_sw_desc *sdesc, *next;

	msgdma_copy_one(mdev, desc);

	list_for_each_entry_safe(sdesc, next, &desc->tx_list, node)
		msgdma_copy_one(mdev, sdesc);
}
/* …（同文件无关代码省略）… */
static void msgdma_start_transfer(struct msgdma_device *mdev)
{
	struct msgdma_sw_desc *desc;

	if (!mdev->idle)
		return;

	desc = list_first_entry_or_null(&mdev->pending_list,
					struct msgdma_sw_desc, node);
	if (!desc)
		return;

	list_splice_tail_init(&mdev->pending_list, &mdev->active_list);
	msgdma_copy_desc_to_fifo(mdev, desc);
}
/* …（同文件无关代码省略）… */
static void msgdma_chan_desc_cleanup(struct msgdma_device *mdev)
{
	struct msgdma_sw_desc *desc, *next;
	unsigned long irqflags;

	spin_lock_irqsave(&mdev->lock, irqflags);

	list_for_each_entry_safe(desc, next, &mdev->done_list, node) {
		struct dmaengine_desc_callback cb;

		dmaengine_desc_get_callback(&desc->async_tx, &cb);
		if (dmaengine_desc_callback_valid(&cb)) {
			spin_unlock_irqrestore(&mdev->lock, irqflags);
			dmaengine_desc_callback_invoke(&cb, NULL);
			spin_lock_irqsave(&mdev->lock, irqflags);
		}

		/* Run any dependencies, then free the descriptor */
		msgdma_free_descriptor(mdev, desc);
	}

	spin_unlock_irqrestore(&mdev->lock, irqflags);
}
/* …（同文件无关代码省略）… */
static void msgdma_complete_descriptor(struct msgdma_device *mdev)
{
	struct msgdma_sw_desc *desc;

	desc = list_first_entry_or_null(&mdev->active_list,
					struct msgdma_sw_desc, node);
	if (!desc)
		return;
	list_del(&desc->node);
	dma_cookie_complete(&desc->async_tx);
	list_add_tail(&desc->node, &mdev->done_list);
}
/* …（同文件无关代码省略）… */
static void msgdma_tasklet(struct tasklet_struct *t)
{
	struct msgdma_device *mdev = from_tasklet(mdev, t, irq_tasklet);
	u32 count;
	u32 __maybe_unused size;
	u32 __maybe_unused status;
	unsigned long flags;

	spin_lock_irqsave(&mdev->lock, flags);

	if (mdev->resp) {
		/* Read number of responses that are available */
		count = ioread32(mdev->csr + MSGDMA_CSR_RESP_FILL_LEVEL);
		dev_dbg(mdev->dev, "%s (%d): response count=%d\n",
			__func__, __LINE__, count);
	} else {
		count = 1;
	}

	while (count--) {
		/*
		 * Read both longwords to purge this response from the FIFO
		 * On Avalon-MM implementations, size and status do not
		 * have any real values, like transferred bytes or error
		 * bits. So we need to just drop these values.
		 */
		if (mdev->resp) {
			size = ioread32(mdev->resp +
					MSGDMA_RESP_BYTES_TRANSFERRED);
			status = ioread32(mdev->resp +
					MSGDMA_RESP_STATUS);
		}

		msgdma_complete_descriptor(mdev);
	}

	spin_unlock_irqrestore(&mdev->lock, flags);

	msgdma_chan_desc_cleanup(mdev);
}
/* …（同文件无关代码省略）… */
static irqreturn_t msgdma_irq_handler(int irq, void *data)
{
	struct msgdma_device *mdev = data;
	u32 status;

	status = ioread32(mdev->csr + MSGDMA_CSR_STATUS);
	if ((status & MSGDMA_CSR_STAT_BUSY) == 0) {
		/* Start next transfer if the DMA controller is idle */
		spin_lock(&mdev->lock);
		mdev->idle = true;
		msgdma_start_transfer(mdev);
		spin_unlock(&mdev->lock);
	}

	tasklet_schedule(&mdev->irq_tasklet);

	/* Clear interrupt in mSGDMA controller */
	iowrite32(MSGDMA_CSR_STAT_IRQ, mdev->csr + MSGDMA_CSR_STATUS);

	return IRQ_HANDLED;
}
/* …（同文件无关代码省略）… */
	if (mdev->irq < 0)
		return -ENXIO;

	ret = devm_request_irq(&pdev->dev, mdev->irq, msgdma_irq_handler,
			       0, dev_name(&pdev->dev), mdev);
	if (ret)
		return ret;

	tasklet_setup(&mdev->irq_tasklet, msgdma_tasklet);

	dma_cookie_init(&mdev->dmachan);
/* …（同文件无关代码省略）… */
	INIT_LIST_HEAD(&mdev->done_list);
	INIT_LIST_HEAD(&mdev->free_list);

	dma_dev = &mdev->dmadev;

	/* Set DMA capabilities */
