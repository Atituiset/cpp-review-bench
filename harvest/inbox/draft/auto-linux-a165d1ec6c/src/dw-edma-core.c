// AUTO-DRAFT from torvalds/linux PR #ae3690eb2a994dd74a43639d6bf800b51f18b48c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/dmaengine.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/dma/edma.h>
#include <linux/dma-mapping.h>
/* …（同文件无关代码省略）… */
static inline
struct dw_edma_desc *vd2dw_edma_desc(struct virt_dma_desc *vd)
{
	return container_of(vd, struct dw_edma_desc, vd);
}

enum dw_edma_irq_event {
	DW_EDMA_IRQ_DONE	= BIT(0),
	DW_EDMA_IRQ_ABORT	= BIT(1),
};

static inline
u64 dw_edma_get_pci_address(struct dw_edma_chan *chan, phys_addr_t cpu_addr)
{
	struct dw_edma_chip *chip = chan->dw->chip;

	if (chip->ops->pci_address)
		return chip->ops->pci_address(chip->dev, cpu_addr);

	return cpu_addr;
}
/* …（同文件无关代码省略）… */
static struct dw_edma_desc *
dw_edma_alloc_desc(struct dw_edma_chan *chan, size_t nburst)
{
	struct dw_edma_desc *desc;

	desc = kzalloc_flex(*desc, burst, nburst, GFP_NOWAIT);
	if (unlikely(!desc))
		return NULL;

	desc->chan = chan;
	desc->nburst = nburst;
	desc->cb = true;

	return desc;
}
/* …（同文件无关代码省略）… */
	kfree(vd2dw_edma_desc(vdesc));
}

static void dw_edma_core_start(struct dw_edma_desc *desc, bool first)
{
	struct dw_edma_chan *chan = desc->chan;
	size_t i = 0;

	if (chan->non_ll) {
		chan->dw->core->non_ll_start(chan, &desc->burst[desc->start_burst]);
		desc->done_burst = desc->start_burst;
		desc->start_burst += 1;
		return;
	}

	for (i = 0; i + desc->start_burst < desc->nburst; i++) {
		u32 idx = i + desc->start_burst;

		if (i == chan->ll_max)
			break;

		dw_edma_core_ll_data(chan, &desc->burst[idx],
				     i, desc->cb,
				     idx == desc->nburst - 1 || i == chan->ll_max - 1);
	}

	desc->done_burst = desc->start_burst;
	desc->start_burst += i;

	dw_edma_core_ll_link(chan, i, desc->cb, chan->ll_region.paddr);

	if (first)
		dw_edma_core_ch_enable(chan);

	dw_edma_core_ch_doorbell(chan);
}

static int dw_edma_start_transfer(struct dw_edma_chan *chan)
{
	struct dw_edma_desc *desc;
	struct virt_dma_desc *vd;

	vd = vchan_next_desc(&chan->vc);
	if (!vd)
		return 0;

	desc = vd2dw_edma_desc(vd);
	if (!desc)
		return 0;

	dw_edma_core_start(desc, !desc->start_burst);

	desc->cb = !desc->cb;

	return 1;
}
/* …（同文件无关代码省略）… */
static void dw_edma_terminate_vdesc(struct virt_dma_desc *vd)
{
	list_del(&vd->node);
	dma_cookie_complete(&vd->tx);
	vchan_terminate_vdesc(vd);
}
/* …（同文件无关代码省略）… */
static void dw_edma_terminate_vdesc_list(struct list_head *head)
{
	struct virt_dma_desc *vd, *_vd;

	list_for_each_entry_safe(vd, _vd, head, node)
		dw_edma_terminate_vdesc(vd);
}
/* …（同文件无关代码省略）… */
static void dw_edma_terminate_all_descs(struct dw_edma_chan *chan)
{
	/*
	 * This order must not be reversed. Cookies are assigned when
	 * descriptors are submitted, so desc_issued contains older cookies
	 * than desc_submitted. Completing desc_submitted first could move
	 * chan->vc.chan.completed_cookie backwards when desc_issued is
	 * terminated afterwards.
	 */
	dw_edma_terminate_vdesc_list(&chan->vc.desc_issued);
	dw_edma_terminate_vdesc_list(&chan->vc.desc_submitted);
}

static void dw_edma_device_caps(struct dma_chan *dchan,
				struct dma_slave_caps *caps)
{
/* …（同文件无关代码省略）… */
						  DW_EDMA_CH_IRQ_REMOTE;
}

static int dw_edma_device_config(struct dma_chan *dchan,
				 struct dma_slave_config *config)
{
	struct dw_edma_chan *chan = dchan2dw_edma_chan(dchan);
	bool cfg_non_ll;
	int non_ll = 0;

	chan->non_ll = false;
	if (chan->dw->chip->mf == EDMA_MF_HDMA_NATIVE) {
		if (config->peripheral_config &&
		    config->peripheral_size != sizeof(int)) {
			dev_err(dchan->device->dev,
				"config param peripheral size mismatch\n");
			return -EINVAL;
		}

		/*
		 * When there is no valid LLP base address available then the
		 * default DMA ops will use the non-LL mode.
		 *
		 * Cases where LL mode is enabled and client wants to use the
		 * non-LL mode then also client can do so via providing the
		 * peripheral_config param.
		 */
		cfg_non_ll = chan->dw->chip->cfg_non_ll;
		if (config->peripheral_config) {
			non_ll = *(int *)config->peripheral_config;

			if (cfg_non_ll && !non_ll) {
				dev_err(dchan->device->dev, "invalid configuration\n");
				return -EINVAL;
			}
		}

		if (cfg_non_ll || non_ll)
			chan->non_ll = true;
	} else if (config->peripheral_config) {
		dev_err(dchan->device->dev,
			"peripheral config param applicable only for HDMA\n");
		return -EINVAL;
	}

	memcpy(&chan->config, config, sizeof(*config));
	chan->configured = true;

/* …（同文件无关代码省略）… */
static struct dma_slave_config *
dw_edma_device_get_config(struct dma_chan *dchan,
			  struct dma_slave_config *config)
{
	struct dw_edma_chan *chan;

	if (config)
		return config;

	chan = dchan2dw_edma_chan(dchan);

	return &chan->config;
}
/* …（同文件无关代码省略）… */
	else if (chan->request != EDMA_REQ_NONE)
		err = -EPERM;
	else
		chan->request = EDMA_REQ_PAUSE;

	return err;
}
/* …（同文件无关代码省略）… */
		chan->status = EDMA_ST_BUSY;
		if (!dw_edma_start_transfer(chan))
			chan->status = EDMA_ST_IDLE;
	}

	return err;
/* …（同文件无关代码省略）… */
static int dw_edma_device_terminate_all(struct dma_chan *dchan)
{
	struct dw_edma_chan *chan = dchan2dw_edma_chan(dchan);
	int err = 0;

	guard(spinlock_irqsave)(&chan->vc.lock);

	if (!chan->configured) {
		dw_edma_terminate_all_descs(chan);
	} else if (chan->status == EDMA_ST_PAUSE) {
		dw_edma_terminate_all_descs(chan);
		chan->status = EDMA_ST_IDLE;
	} else if (chan->status == EDMA_ST_IDLE) {
		dw_edma_terminate_all_descs(chan);
	} else if (dw_edma_core_ch_status(chan) == DMA_COMPLETE) {
		/*
		 * The channel is in a false BUSY state, probably didn't
		 * receive or lost an interrupt
		 */
		dw_edma_terminate_all_descs(chan);
		chan->status = EDMA_ST_IDLE;
	} else if (chan->request > EDMA_REQ_PAUSE) {
		err = -EPERM;
	} else {
		chan->request = EDMA_REQ_STOP;
	}
	if (chan->status == EDMA_ST_IDLE)
		chan->re
