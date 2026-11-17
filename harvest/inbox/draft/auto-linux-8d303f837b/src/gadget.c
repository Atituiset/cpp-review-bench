// AUTO-DRAFT from torvalds/linux PR #f11fe7603df07fbb9fa02b0d385c1696c87a6dab
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
static void dwc3_ep_inc_trb(u8 *index)
{
	(*index)++;
	if (*index == (DWC3_TRB_NUM - 1))
		*index = 0;
}
/* …（同文件无关代码省略）… */
static void dwc3_ep_inc_enq(struct dwc3_ep *dep)
{
	dwc3_ep_inc_trb(&dep->trb_enqueue);
}
/* …（同文件无关代码省略）… */
static void dwc3_ep_inc_deq(struct dwc3_ep *dep)
{
	dwc3_ep_inc_trb(&dep->trb_dequeue);
}
/* …（同文件无关代码省略）… */
static void dwc3_gadget_del_and_unmap_request(struct dwc3_ep *dep,
		struct dwc3_request *req, int status)
{
	struct dwc3			*dwc = dep->dwc;

	list_del(&req->list);
	req->remaining = 0;
	req->num_trbs = 0;

	if (req->request.status == -EINPROGRESS)
		req->request.status = status;

	if (req->trb)
		usb_gadget_unmap_request_by_dev(dwc->sysdev,
				&req->request, req->direction);

	req->trb = NULL;
	trace_dwc3_gadget_giveback(req);

	if (dep->number > 1)
		pm_runtime_put(dwc->dev);
}
/* …（同文件无关代码省略）… */
void dwc3_gadget_giveback(struct dwc3_ep *dep, struct dwc3_request *req,
		int status)
{
	struct dwc3			*dwc = dep->dwc;

	/*
	 * The request might have been processed and completed while the
	 * spinlock was released. Skip processing if already completed.
	 */
	if (req->status == DWC3_REQUEST_STATUS_COMPLETED)
		return;

	dwc3_gadget_del_and_unmap_request(dep, req, status);
	req->status = DWC3_REQUEST_STATUS_COMPLETED;

	spin_unlock(&dwc->lock);
	usb_gadget_giveback_request(&dep->endpoint, &req->request);
	spin_lock(&dwc->lock);
}
/* …（同文件无关代码省略）… */
int dwc3_send_gadget_ep_cmd(struct dwc3_ep *dep, unsigned int cmd,
		struct dwc3_gadget_ep_cmd_params *params)
{
	const struct usb_endpoint_descriptor *desc = dep->endpoint.desc;
	struct dwc3		*dwc = dep->dwc;
	u32			timeout = 5000;
	u32			saved_config = 0;
	u32			reg;

	int			cmd_status = 0;
	int			ret = -EINVAL;
	u8			epnum = dep->number;

	/*
	 * When operating in USB 2.0 speeds (HS/FS), if GUSB2PHYCFG.ENBLSLPM or
	 * GUSB2PHYCFG.SUSPHY is set, it must be cleared before issuing an
	 * endpoint command.
	 *
	 * Save and clear both GUSB2PHYCFG.ENBLSLPM and GUSB2PHYCFG.SUSPHY
	 * settings. Restore them after the command is completed.
	 *
	 * DWC_usb3 3.30a and DWC_usb31 1.90a programming guide section 3.2.2
	 */
	if (dwc->gadget->speed <= USB_SPEED_HIGH ||
	    DWC3_DEPCMD_CMD(cmd) == DWC3_DEPCMD_ENDTRANSFER) {
		reg = dwc3_readl(dwc, DWC3_GUSB2PHYCFG(0));
		if (unlikely(reg & DWC3_GUSB2PHYCFG_SUSPHY)) {
			saved_config |= DWC3_GUSB2PHYCFG_SUSPHY;
			reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;
		}

		if (reg & DWC3_GUSB2PHYCFG_ENBLSLPM) {
			saved_config |= DWC3_GUSB2PHYCFG_ENBLSLPM;
			reg &= ~DWC3_GUSB2PHYCFG_ENBLSLPM;
		}

		if (saved_config)
			dwc3_writel(dwc, DWC3_GUSB2PHYCFG(0), reg);
	}

	/*
	 * For some commands such as Update Transfer command, DEPCMDPARn
	 * registers are reserved. Since the driver often sends Update Transfer
	 * command, don't write to DEPCMDPARn to avoid register write delays and
	 * improve performance.
	 */
	if (DWC3_DEPCMD_CMD(cmd) != DWC3_DEPCMD_UPDATETRANSFER) {
		dwc3_writel(dwc, DWC3_DEPCMDPAR0(epnum), params->param0);
		dwc3_writel(dwc, DWC3_DEPCMDPAR1(epnum), params->param1);
		dwc3_writel(dwc, DWC3_DEPCMDPAR2(epnum), params->param2);
	}

	/*
	 * Synopsys Databook 2.60a states in section 6.3.2.5.6 of that if we're
	 * not relying on XferNotReady, we can make use of a special "No
	 * Response Update Transfer" command where we should clear both CmdAct
	 * and CmdIOC bits.
	 *
	 * With this, we don't need to wait for command completion and can
	 * straight away issue further commands to the endpoint.
	 *
	 * NOTICE: We're making an assumption that control endpoints will never
	 * make use of Update Transfer command. This is a safe assumption
	 * because we can never have more than one request at a time with
	 * Control Endpoints. If anybody changes that assumption, this chunk
	 * needs to be updated accordingly.
	 */
	if (DWC3_DEPCMD_CMD(cmd) == DWC3_DEPCMD_UPDATETRANSFER &&
			!usb_endpoint_xfer_isoc(desc))
		cmd &= ~(DWC3_DEPCMD_CMDIOC | DWC3_DEPCMD_CMDACT);
	else
		cmd |= DWC3_DEPCMD_CMDACT;

	dwc3_writel(dwc, DWC3_DEPCMD(epnum), cmd);

	if (!(cmd & DWC3_DEPCMD_CMDACT) ||
		(DWC3_DEPCMD_CMD(cmd) == DWC3_DEPCMD_ENDTRANSFER &&
		!(cmd & DWC3_DEPCMD_CMDIOC))) {
		ret = 0;
		goto skip_status;
	}

	do {
		reg = dwc3_readl(dwc, DWC3_DEPCMD(epnum));
		if (!(reg & DWC3_DEPCMD_CMDACT)) {
			cmd_status = DWC3_DEPCMD_STATUS(reg);

			switch (cmd_status) {
			case 0:
				ret = 0;
				break;
			case DEPEVT_TRANSFER_NO_RESOURCE:
				dev_WARN(dwc->dev, "No resource for %s\n",
					 dep->name);
				ret = -EINVAL;
				break;
			case DEPEVT_TRANSFER_BUS_EXPIRY:
				/*
				 * SW issues START TRANSFER command to
				 * isochronous ep with future frame interval. If
				 * future interval time has already passed when
				 * core receives the command, it will respond
				 * with an error status of 'Bus Expiry'.
				 *
				 * Instead of always returning -EINVAL, let's
				 * give a hint to the gadget driver that this is
				 * the case by returning -EAGAIN.
				 */
				ret = -EAGAIN;
				break;
			default:
				dev_WARN(dwc->dev, "UNKNOWN cmd status\n");
			}

			break;
		}
	} while (--timeout);

	if (timeout == 0) {
		ret = -ETIMEDOUT;
		cmd_status = -ETIMEDOUT;
	}

skip_status:
	trace_dwc3_gadget_ep_cmd(dep, cmd, params, cmd_status);

	if (DWC3_DEPCMD_CMD(cmd) == DWC3_DEPCMD_STARTTRANSFER) {
		if (ret == 0)
			dep->flags |= DWC3_EP_TRANSFER_STARTED;

		if (ret != -ETIMEDOUT)
			dwc3_gadget_ep_get_transfer_index(dep);
	}

	if (DWC3_DEPCMD_CMD(cmd) == DWC3_DEPCMD_ENDTRANSFER &&
	    !(cmd & DWC3_DEPCMD_CMDIOC))
		mdelay(1);

	if (saved_config) {
		reg = dwc3_readl(dwc, DWC3_GUSB2PHYCFG(0));
		reg |= saved_config;
		dwc3_writel(dwc, DWC3_GUSB2PHYCFG(0), reg);
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static dma_addr_t dwc3_trb_dma_offset(struct dwc3_ep *dep,
		struct dwc3_trb *trb)
{
	u32		offset = (char *) trb - (char *) dep->trb_pool;

	return dep->trb_pool_dma + offset;
}
/* …（同文件无关代码省略）… */
static int dwc3_gadget_set_xfer_resource(struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;
	int ret;

	if (dep->flags & DWC3_EP_RESOURCE_ALLOCATED)
		return 0;

	memset(&params, 0x00, sizeof(params));

	params.param0 = DWC3_DEPXFERCFG_NUM_XFER_RES(1);

	ret = dwc3_send_gadget_ep_cmd(dep, DWC3_DEPCMD_SETTRANSFRESOURCE,
			&params);
	if (ret)
		return ret;

	dep->flags |= DWC3_EP_RESOURCE_ALLOCATED;
	return 0;
}
/* …（同文件无关代码省略）… */
static int dwc3_gadget_set_ep_config(struct dwc3_ep *dep, unsigned int action)
{
	const struct usb_ss_ep_comp_descriptor *comp_desc;
	const struct usb_endpoint_descriptor *desc;
	struct dwc3_gadget_ep_cmd_params params;
	struct dwc3 *dwc = dep->dwc;

	comp_desc = dep->endpoint.comp_desc;
	desc = dep->endpoint.desc;

	memset(&params, 0x00, sizeof(params));

	params.param0 = DWC3_DEPCFG_EP_TYPE(usb_endpoint_type(desc))
		| DWC3_DEPCFG_MAX_PACKET_SIZE(usb_endpoint_maxp(desc));

	/* Burst size is only needed in SuperSpeed mode */
	if (dwc->gadget->speed >= USB_SPEED_SUPER) {
		u32 burst = dep->endpoint.maxburst;

		params.param0 |= DWC3_DEPCFG_BURST_SIZE(burst - 1);
	}

	params.param0 |= action;
	if (action == DWC3_DEPCFG_ACTION_RESTORE)
		params.param2 |= dep->saved_state;

	if (usb_endpoint_xfer_control(desc))
		params.param1 = DWC3_DEPCFG_XFER_COMPLETE_EN;

	if (dep->number <= 1 || usb_endpoint_xfer_isoc(desc))
		params.param1 |= DWC3_DEPCFG_XFER_NOT_READY_EN;

	if (usb_ss_max_streams(comp_desc) && usb_endpoint_xfer_bulk(desc)) {
		params.param1 |= DWC3_DEPCFG_STREAM_CAPABLE
			| DWC3_DEPCFG_XFER_COMPLETE_EN
			| DWC3_DEPCFG_STREAM_EVENT_EN;
		dep->stream_capable = true;
	}

	if (!usb_endpoint_xfer_control(desc))
		params.param1 |= DWC3_DEPCFG_XFER_IN_PROGRESS_EN;

	/*
	 * We are doing 1:1 mapping for endpoints, meaning
	 * Physical Endpoints 2 maps to Logical Endpoint 2 and
	 * so on. We consider the direction bit as part of the physical
	 * endpoint number. So USB endpoint 0x81 is 0x03.
	 */
	params.param1 |= DWC3_DEPCFG_EP_NUMBER(dep->number);

	/*
	 * We must use the lower 16 TX FIFOs even though
	 * HW might have more
	 */
	if (dep->direction)
		params.param0 |= DWC3_DEPCFG_FIFO_NUMBER(dep->number >> 1);

	if (desc->bInterval) {
		u8 bInterval_m1;

		/*
		 * Valid range for DEPCFG.bInterval_m1 is from 0 to 13.
		 *
		 * NOTE: The programming guide incorrectly stated bInterval_m1
		 * must be set to 0 when operating in fullspeed. Internally the
		 * controller does not have this limitation. See DWC_usb3x
		 * programming guide section 3.2.2.1.
		 */
		bInterval_m1 = min_t(u8, desc->bInterval - 1, 13);

		if (usb_endpoint_type(desc) == USB_ENDPOINT_XFER_INT &&
		    dwc->gadget->speed == USB_SPEED_FULL)
			dep->interval = desc->bInterval;
		else
			dep->interval = 1 << (desc->bInterval - 1);

		params.param1 |= DWC3_DEPCFG_BINTERVAL_M1(bInterval_m1);
	}

	return dwc3_send_gadget_ep_cmd(dep, DWC3_DEPCMD_SETEPCONFIG, &params);
}
/* …（同文件无关代码省略）… */
static int dwc3_gadget_calc_tx_fifo_size(struct dwc3 *dwc, int mult)
{
	int max_packet = 1024;
	int fifo_size;
	int mdwidth;

	mdwidth = dwc3_mdwidth(dwc);

	/* MDWIDTH is represented in bits, we need it in bytes */
	mdwidth >>= 3;

	if (DWC3_VER_IS_PRIOR(DWC3, 290A))
		fifo_size = mult * (max_packet / mdwidth) + 1;
	else
		fifo_size = mult * ((max_packet + mdwidth) / mdwidth) + 1;
	return fifo_size;
}
/* …（同文件无关代码省略）… */
static int dwc3_gadget_calc_ram_depth(struct dwc3 *dwc)
{
	int ram_depth;
	int fifo_0_start;
	bool is_single_port_ram;

	/* Check supporting RAM type by HW */
	is_single_port_ram = DWC3_SPRAM_TYPE(dwc->hwparams.hwparams1);

	/*
	 * If a single port RAM is utilized, then allocate TxFIFOs from
	 * RAM0. otherwise, allocate them from RAM1.
	 */
	ram_depth = is_single_port_ram ? DWC3_RAM0_DEPTH(dwc->hwparams.hwparams6) :
			DWC3_RAM1_DEPTH(dwc->hwparams.hwparams7);

	/*
	 * In a single port RAM configuration, the available RAM is shared
	 * between the RX and TX FIFOs. This means that the txfifo can begin
	 * at a non-zero address.
	 */
	if (is_single_port_ram) {
		u32 reg;

		/* Check if TXFIFOs start at non-zero addr */
		reg = dwc3_readl(dwc, DWC3_GTXFIFOSIZ(0));
		fifo_0_start = DWC3_GTXFIFOSIZ_TXFSTADDR(reg);

		ram_depth -= (fifo_0_start >> 16);
	}

	return ram_depth;
}
/* …（同文件无关代码省略）… */
static int dwc3_gadget_resize_tx_fifos(struct dwc3_ep *dep)
{
	struct dwc3 *dwc = dep->dwc;
	int fifo_0_start;
	int ram_depth;
	int fifo_size;
	int min_depth;
	int num_in_ep;
	int remaining;
	int num_fifos = 1;
	int fifo;
	int tmp;

	if (!dwc->do_fifo_resize)
		return 0;

	/* resize IN endpoints except ep0 */
	if (!usb_endpoint_dir_in(dep->endpoint.desc) || dep->number <= 1)
		return 0;

	/* bail if already resized */
	if (dep->flags & DWC3_EP_TXFIFO_RESIZED)
		return 0;

	ram_depth = dwc3_gadget_calc_ram_depth(dwc);

	switch (dwc->gadget->speed) {
	case USB_SPEED_SUPER_PLUS:
	case USB_SPEED_SUPER:
		if (usb_endpoint_xfer_bulk(dep->endpoint.desc) ||
		    usb_endpoint_xfer_isoc(dep->endpoint.desc))
			num_fifos = min_t(unsigned int,
					  dep->endpoint.maxburst,
					  dwc->tx_fifo_resize_max_num);
		break;
	case USB_SPEED_HIGH:
		if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			num_fifos = min_t(unsigned int,
					  usb_endpoint_maxp_mult(dep->endpoint.desc) + 1,
					  dwc->tx_fifo_resize_max_num);
			break;
		}
		fallthrough;
	case USB_SPEED_FULL:
		if (usb_endpoint_xfer_bulk(dep->endpoint.desc))
			num_fifos = 2;
		break;
	default:
		break;
	}

	/* FIFO size for a single buffer */
	fifo = dwc3_gadget_calc_tx_fifo_size(dwc, 1);

	/* Calculate the number of remaining EPs w/o any FIFO */
	num_in_ep = dwc->max_cfg_eps;
	num_in_ep -= dwc->num_ep_resized;

	/* Reserve at least one FIFO for the number of IN EPs */
	min_depth = num_in_ep * (fifo + 1);
	remaining = ram_depth - min_depth - dwc->last_fifo_depth;
	remaining = max_t(int, 0, remaining);
	/*
	 * We've already reserved 1 FIFO per EP, so check what we can fit in
	 * addition to it.  If there is not enough remaining space, allocate
	 * all the remaining space to the EP.
	 */
	fifo_size = (num_fifos - 1) * fifo;
	if (remaining < fifo_size)
		fifo_size = remaining;

	fifo_size += fifo;
	/* Last increment according to the TX FIFO size equation */
	fifo_size++;

	/* Check if TXFIFOs start at non-zero addr */
	tmp = dwc3_readl(dwc, DWC3_GTXFIFOSIZ(0));
	fifo_0_start = DWC3_GTXFIFOSIZ_TXFSTADDR(tmp);

	fifo_size |= (fifo_0_start + (dwc->last_fifo_depth << 16));
	if (DWC3_IP_IS(DWC3))
		dwc->last_fifo_depth += DWC3_GTXFIFOSIZ_TXFDEP(fifo_size);
	else
		dwc->last_fifo_depth += DWC31_GTXFIFOSIZ_TXFDEP(fifo_size);

	/* Check fifo size allocation doesn't exceed available RAM size. */
	if (dwc->last_fifo_depth >= ram_depth) {
		dev_err(dwc->dev, "Fifosize(%d) > RAM size(%d) %s depth:%d\n",
			dwc->last_fifo_depth, ram_depth,
			dep->endpoint.name, fifo_size);
		if (DWC3_IP_IS(DWC3))
			fifo_size = DWC3_GTXFIFOSIZ_TXFDEP(fifo_size);
		else
			fifo_size = DWC31_GTXFIFOSIZ_TXFDEP(fifo_size);

		dwc->last_fifo_depth -= fifo_size;
		return -ENOMEM;
	}

	dwc3_writel(dwc, DWC3_GTXFIFOSIZ(dep->number >> 1), fifo_size);
	dep->flags |= DWC3_EP_TXFIFO_RESIZED;
	dwc->num_ep_resized++;

	return 0;
}

/**
 * __dwc3_gadget_ep_enable - initializes a hw endpoint
 * @dep: endpoint to be initialized
/* …（同文件无关代码省略）… */
static int __dwc3_gadget_ep_enable(struct dwc3_ep *dep, unsigned int action)
{
	const struct usb_endpoint_descriptor *desc = dep->endpoint.desc;
	struct dwc3		*dwc = dep->dwc;

	u32			reg;
	int			ret;

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		ret = dwc3_gadget_resize_tx_fifos(dep);
		if (ret)
			return ret;
	}

	ret = dwc3_gadget_set_ep_config(dep, action);
	if (ret)
		return ret;

	ret = dwc3_gadget_set_xfer_resource(dep);
	if (ret)
		return ret;

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		struct dwc3_trb	*trb_st_hw;
		struct dwc3_trb	*trb_link;

		dep->type = usb_endpoint_type(desc);
		dep->flags |= DWC3_EP_ENABLED;

		reg = dwc3_readl(dwc, DWC3_DALEPENA);
		reg |= DWC3_DALEPENA_EP(dep->number);
		dwc3_writel(dwc, DWC3_DALEPENA, reg);

		dep->trb_dequeue = 0;
		dep->trb_enqueue = 0;

		if (usb_endpoint_xfer_control(desc))
			goto out;

		/* Initialize the TRB ring */
		memset(dep->trb_pool, 0,
		       sizeof(struct dwc3_trb) * DWC3_TRB_NUM);

		/* Link TRB. The HWO bit is never reset */
		trb_st_hw = &dep->trb_pool[0];

		trb_link = &dep->trb_pool[DWC3_TRB_NUM - 1];
		trb_link->bpl = lower_32_bits(dwc3_trb_dma_offset(dep, trb_st_hw));
		trb_link->bph = upper_32_bits(dwc3_trb_dma_offset(dep, trb_st_hw));
		trb_link->ctrl |= DWC3_TRBCTL_LINK_TRB;
		trb_link->ctrl |= DWC3_TRB_CTRL_HWO;
	}

	/*
	 * Issue StartTransfer here with no-op TRB so we can always rely on No
	 * Response Update Transfer command.
	 */
	if (usb_endpoint_xfer_bulk(desc) ||
			usb_endpoint_xfer_int(desc)) {
		struct dwc3_gadget_ep_cmd_params params;
		struct dwc3_trb	*trb;
		dma_addr_t trb_dma;
		u32 cmd;

		memset(&params, 0, sizeof(params));
		trb = &dep->trb_pool[0];
		trb_dma = dwc3_trb_dma_offset(dep, trb);

		params.param0 = upper_32_bits(trb_dma);
		params.param1 = lower_32_bits(trb_dma);

		cmd = DWC3_DEPCMD_STARTTRANSFER;

		ret = dwc3_send_gadget_ep_cmd(dep, cmd, &params);
		if (ret < 0)
			return ret;

		if (dep->stream_capable) {
			/*
			 * For streams, at start, there maybe a race where the
			 * host primes the endpoint before the function driver
			 * queues a request to initiate a stream. In that case,
			 * the controller will not see the prime to generate the
			 * ERDY and start stream. To workaround this, issue a
			 * no-op TRB as normal, but end it immediately. As a
			 * result, when the function driver queues the request,
			 * the next START_TRANSFER command will cause the
			 * controller to generate an ERDY to initiate the
			 * stream.
			 */
			dwc3_stop_active_transfer(dep, false, true);

			/*
			 * All stream eps will reinitiate stream on NoStream
			 * rejection.
			 *
			 * However, if the controller is capable of
			 * TXF_FLUSH_BYPASS, then IN direction endpoints will
			 * automatically restart the stream without the driver
			 * initiation.
			 */
			if (!dep->direction ||
			    !(dwc->hwparams.hwparams9 &
			      DWC3_GHWPARAMS9_DEV_TXF_FLUSH_BYPASS))
				dep->flags |= DWC3_EP_FORCE_RESTART_STREAM;
		}
	}

out:
	trace_dwc3_gadget_ep_enable(dep);

	return 0;
}

void dwc3_remove_requests(struct dwc3 *dwc, struct dwc3_ep *dep, int status)
{
	struct dwc3_request		*req;

	dwc3_stop_active_transfer(dep, false, false);

	/* If endxfer is delayed, avoid unmapping requests */
	if (dep->flags & DWC3_EP_DELAY_STOP)
		return;

	/* - giveback all requests to gadget driver */
	while (!list_empty(&dep->started_list)) {
		req = next_request(&dep->started_list);
/* …（同文件无关代码省略）… */
	reg &= ~DWC3_DALEPENA_EP(dep->number);
	dwc3_writel(dwc, DWC3_DALEPENA, reg);

	dwc3_remove_requests(dwc, dep, -ESHUTDOWN);

	dep->stream_capable = false;
	dep->type = 0;
/* …（同文件无关代码省略）… */
	 */
	if (dep->flags & DWC3_EP_DELAY_STOP)
		mask |= (DWC3_EP_DELAY_STOP | DWC3_EP_TRANSFER_STARTED);
	dep->flags &= mask;

	/* Clear out the ep descriptors for non-ep0 */
/* …（同文件无关代码省略）… */
static u32 dwc3_calc_trbs_left(struct dwc3_ep *dep)
{
	u8			trbs_left;

	/*
	 * If the enqueue & dequeue are equal then the TRB ring is either full
	 * or empty. It's considered full when there are DWC3_TRB_NUM-1 of TRBs
	 * pending to be processed by the driver.
	 */
	if (dep->trb_enqueue == dep->trb_dequeue) {
		struct dwc3_request *req;

		/*
		 * If there is any request remained in the started_list with
		 * active TRBs at this point, then there is no TRB available.
		 */
		req = next_request(&dep->started_list);
		if (req && req->num_trbs)
			return 0;

		return DWC3_TRB_NUM - 1;
	}

	trbs_left = dep->trb_dequeue - dep->trb_enqueue;
	trbs_left &= (DWC3_TRB_NUM - 1);

	if (dep->trb_dequeue < dep->trb_enqueue)
		trbs_left--;

	return trbs_left;
}
/* …（同文件无关代码省略）… */
static void dwc3_prepare_one_trb(struct dwc3_ep *dep,
		struct dwc3_request *req, unsigned int trb_length,
		unsigned int chain, unsigned int node, bool use_bounce_buffer,
		bool must_interrupt)
{
	struct dwc3_trb		*trb;
	dma_addr_t		dma;
	unsigned int		stream_id = req->request.stream_id;
	unsigned int		short_not_ok = req->request.short_not_ok;
	unsigned int		no_interrupt = req->request.no_interrupt;
	unsigned int		is_last = req->request.is_last;
	struct dwc3		*dwc = dep->dwc;
	struct usb_gadget	*gadget = dwc->gadget;
	enum usb_device_speed	speed = gadget->speed;

	if (use_bounce_buffer)
		dma = dep->dwc->bounce_addr;
	else if (req->request.num_sgs > 0)
		dma = sg_dma_address(req->start_sg);
	else
		dma = req->request.dma;

	trb = &dep->trb_pool[dep->trb_enqueue];

	if (!req->trb) {
		dwc3_gadget_move_started_request(req);
		req->trb = trb;
		req->trb_dma = dwc3_trb_dma_offset(dep, trb);
	}

	req->num_trbs++;

	trb->size = DWC3_TRB_SIZE_LENGTH(trb_length);
	trb->bpl = lower_32_bits(dma);
	trb->bph = upper_32_bits(dma);

	switch (usb_endpoint_type(dep->endpoint.desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		trb->ctrl = DWC3_TRBCTL_CONTROL_SETUP;
		break;

	case USB_ENDPOINT_XFER_ISOC:
		if (!node) {
			trb->ctrl = DWC3_TRBCTL_ISOCHRONOUS_FIRST;

			/*
			 * USB Specification 2.0 Section 5.9.2 states that: "If
			 * there is only a single transaction in the microframe,
			 * only a DATA0 data packet PID is used.  If there are
			 * two transactions per microframe, DATA1 is used for
			 * the first transaction data packet and DATA0 is used
			 * for the second transaction data packet.  If there are
			 * three transactions per microframe, DATA2 is used for
			 * the first transaction data packet, DATA1 is used for
			 * the second, and DATA0 is used for the third."
			 *
			 * IOW, we should satisfy the following cases:
			 *
			 * 1) length <= maxpacket
			 *	- DATA0
			 *
			 * 2) maxpacket < length <= (2 * maxpacket)
			 *	- DATA1, DATA0
			 *
			 * 3) (2 * maxpacket) < length <= (3 * maxpacket)
			 *	- DATA2, DATA1, DATA0
			 */
			if (speed == USB_SPEED_HIGH) {
				struct usb_ep *ep = &dep->endpoint;
				unsigned int mult = 2;
				unsigned int maxp = usb_endpoint_maxp(ep->desc);

				if (req->request.length <= (2 * maxp))
					mult--;

				if (req->request.length <= maxp)
					mult--;

				trb->size |= DWC3_TRB_SIZE_PCM1(mult);
			}
		} else {
			trb->ctrl = DWC3_TRBCTL_ISOCHRONOUS;
		}

		if (!no_interrupt && !chain)
			trb->ctrl |= DWC3_TRB_CTRL_ISP_IMI;
		break;

	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		trb->ctrl = DWC3_TRBCTL_NORMAL;
		break;
	default:
		/*
		 * This is only possible with faulty memory because we
		 * checked it already :)
		 */
		dev_WARN(dwc->dev, "Unknown endpoint type %d\n",
				usb_endpoint_type(dep->endpoint.desc));
	}

	/*
	 * Enable Continue on Short Packet
	 * when endpoint is not a stream capable
	 */
	if (usb_endpoint_dir_out(dep->endpoint.desc)) {
		if (!dep->stream_capable)
			trb->ctrl |= DWC3_TRB_CTRL_CSP;

		if (short_not_ok)
			trb->ctrl |= DWC3_TRB_CTRL_ISP_IMI;
	}

	/* All TRBs setup for MST must set CSP=1 when LST=0 */
	if (dep->stream_capable && DWC3_MST_CAPABLE(&dwc->hwparams))
		trb->ctrl |= DWC3_TRB_CTRL_CSP;

	if ((!no_interrupt && !chain) || must_interrupt)
		trb->ctrl |= DWC3_TRB_CTRL_IOC;

	if (chain)
		trb->ctrl |= DWC3_TRB_CTRL_CHN;
	else if (dep->stream_capable && is_last &&
		 !DWC3_MST_CAPABLE(&dwc->hwparams))
		trb->ctrl |= DWC3_TRB_CTRL_LST;

	if (usb_endpoint_xfer_bulk(dep->endpoint.desc) && dep->stream_capable)
		trb->ctrl |= DWC3_TRB_CTRL_SID_SOFN(stream_id);

	/*
	 * As per data book 4.2.3.2TRB Control Bit Rules section
	 *
	 * The controller autonomously checks the HWO field of a TRB to determine if the
	 * entire TRB is valid. Therefore, software must ensure that the rest of the TRB
	 * is valid before setting the HWO field to '1'. In most systems, this means that
	 * software must update the fourth DWORD of a TRB last.
	 *
	 * However there is a possibility of CPU re-ordering here which can cause
	 * controller to observe the HWO bit set prematurely.
	 * Add a write memory barrier to prevent CPU re-ordering.
	 */
	wmb();
	trb->ctrl |= DWC3_TRB_CTRL_HWO;

	dwc3_ep_inc_enq(dep);

	trace_dwc3_prepare_trb(dep, trb);
}
/* …（同文件无关代码省略）… */
static bool dwc3_needs_extra_trb(struct dwc3_ep *dep, struct dwc3_request *req)
{
	unsigned int maxp = usb_endpoint_maxp(dep->endpoint.desc);
	unsigned int rem = req->request.length % maxp;

	if ((req->request.length && req->request.zero && !rem &&
			!usb_endpoint_xfer_isoc(dep->endpoint.desc)) ||
			(!req->direction && rem))
		return true;

	return false;
}
/* …（同文件无关代码省略）… */
static int dwc3_prepare_last_sg(struct dwc3_ep *dep,
		struct dwc3_request *req, unsigned int entry_length,
		unsigned int node)
{
	unsigned int maxp = usb_endpoint_maxp(dep->endpoint.desc);
	unsigned int rem = req->request.length % maxp;
	unsigned int num_trbs = 1;
	bool needs_extra_trb;

	if (dwc3_needs_extra_trb(dep, req))
		num_trbs++;

	if (dwc3_calc_trbs_left(dep) < num_trbs)
		return 0;

	needs_extra_trb = num_trbs > 1;

	/* Prepare a normal TRB */
	if (req->direction || req->request.length)
		dwc3_prepare_one_trb(dep, req, entry_length,
				needs_extra_trb, node, false, false);

	/* Prepare extra TRBs for ZLP and MPS OUT transfer alignment */
	if ((!req->direction && !req->request.length) || needs_extra_trb)
		dwc3_prepare_one_trb(dep, req,
				req->direction ? 0 : maxp - rem,
				false, 1, true, false);

	return num_trbs;
}
/* …（同文件无关代码省略）… */
static int dwc3_prepare_trbs_sg(struct dwc3_ep *dep,
		struct dwc3_request *req)
{
	struct scatterlist *sg = req->start_sg;
	struct scatterlist *s;
	int		i;
	unsigned int length = req->request.length;
	unsigned int remaining = req->num_pending_sgs;
	unsigned int num_queued_sgs = req->request.num_mapped_sgs - remaining;
	unsigned int num_trbs = req->num_trbs;
	bool needs_extra_trb = dwc3_needs_extra_trb(dep, req);

	/*
	 * If we resume preparing the request, then get the remaining length of
	 * the request and resume where we left off.
	 */
	for_each_sg(req->request.sg, s, num_queued_sgs, i)
		length -= sg_dma_len(s);

	for_each_sg(sg, s, remaining, i) {
		unsigned int num_trbs_left = dwc3_calc_trbs_left(dep);
		unsigned int trb_length;
		bool must_interrupt = false;
		bool last_sg = false;

		trb_length = min_t(unsigned int, length, sg_dma_len(s));

		length -= trb_length;

		/*
		 * IOMMU driver is coalescing the list of sgs which shares a
		 * page boundary into one and giving it to USB driver. With
		 * this the number of sgs mapped is not equal to the number of
		 * sgs passed. So mark the chain bit to false if it isthe last
		 * mapped sg.
		 */
		if ((i == remaining - 1) || !length)
			last_sg = true;

		if (!num_trbs_left)
			break;

		if (last_sg) {
			if (!dwc3_prepare_last_sg(dep, req, trb_length, i))
				break;
		} else {
			/*
			 * Look ahead to check if we have enough TRBs for the
			 * next SG entry. If not, set interrupt on this TRB to
			 * resume preparing the next SG entry when more TRBs are
			 * free.
			 */
			if (num_trbs_left == 1 || (needs_extra_trb &&
					num_trbs_left <= 2 &&
					sg_dma_len(sg_next(s)) >= length)) {
				struct dwc3_request *r;

				/* Check if previous requests already set IOC */
				list_for_each_entry(r, &dep->started_list, list) {
					if (r != req && !r->request.no_interrupt)
						break;

					if (r == req)
						must_interrupt = true;
				}
			}

			dwc3_prepare_one_trb(dep, req, trb_length, 1, i, false,
					must_interrupt);
		}

		/*
		 * There can be a situation where all sgs in sglist are not
		 * queued because of insufficient trb number. To handle this
		 * case, update start_sg to next sg to be queued, so that
		 * we have free trbs we can continue queuing from where we
		 * previously stopped
		 */
		if (!last_sg)
			req->start_sg = sg_next(s);

		req->num_pending_sgs--;

		/*
		 * The number of pending SG entries may not correspond to the
		 * number of mapped SG entries. If all the data are queued, then
		 * don't include unused SG entries.
		 */
		if (length == 0) {
			req->num_pending_sgs = 0;
			break;
		}

		if (must_interrupt)
			break;
	}

	return req->num_trbs - num_trbs;
}
/* …（同文件无关代码省略）… */
static int dwc3_prepare_trbs_linear(struct dwc3_ep *dep,
		struct dwc3_request *req)
{
	return dwc3_prepare_last_sg(dep, req, req->request.length, 0);
}
/* …（同文件无关代码省略）… */
static int dwc3_prepare_trbs(struct dwc3_ep *dep)
{
	struct dwc3_request	*req, *n;
	int			ret = 0;

	BUILD_BUG_ON_NOT_POWER_OF_2(DWC3_TRB_NUM);

	/*
	 * We can get in a situation where there's a request in the started list
	 * but there weren't enough TRBs to fully kick it in the first time
	 * around, so it has been waiting for more TRBs to be freed up.
	 *
	 * In that case, we should check if we have a request with pending_sgs
	 * in the started list and prepare TRBs for that request first,
	 * otherwise we will prepare TRBs completely out of order and that will
	 * break things.
	 */
	list_for_each_entry(req, &dep->started_list, list) {
		if (req->num_pending_sgs > 0) {
			ret = dwc3_prepare_trbs_sg(dep, req);
			if (!ret || req->num_pending_sgs)
				return ret;
		}

		if (!dwc3_calc_trbs_left(dep))
			return ret;

		/*
		 * Don't prepare beyond a transfer. In DWC_usb32, its transfer
		 * burst capability may try to read and use TRBs beyond the
		 * active transfer instead of stopping.
		 */
		if (dep->stream_capable && req->request.is_last &&
		    !DWC3_MST_CAPABLE(&dep->dwc->hwparams))
			return ret;
	}

	list_for_each_entry_safe(req, n, &dep->pending_list, list) {
		struct dwc3	*dwc = dep->dwc;

		ret = usb_gadget_map_request_by_dev(dwc->sysdev, &req->request,
						    dep->direction);
		if (ret)
			return ret;

		req->start_sg		= req->request.sg;
		req->num_pending_sgs	= req->request.num_mapped_sgs;

		if (req->num_pending_sgs > 0) {
			ret = dwc3_prepare_trbs_sg(dep, req);
			if (req->num_pending_sgs)
				return ret;
		} else {
			ret = dwc3_prepare_trbs_linear(dep, req);
		}

		if (!ret || !dwc3_calc_trbs_left(dep))
			return ret;

		/*
		 * Don't prepare beyond a transfer. In DWC_usb32, its transfer
		 * burst capability may try to read and use TRBs beyond the
		 * active transfer instead of stopping.
		 */
		if (dep->stream_capable && req->request.is_last &&
		    !DWC3_MST_CAPABLE(&dwc->hwparams))
			return ret;
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static int __dwc3_gadget_kick_transfer(struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;
	struct dwc3_request		*req;
	int				starting;
	int				ret;
	u32				cmd;

	/*
	 * Note that it's normal to have no new TRBs prepared (i.e. ret == 0).
	 * This happens when we need to stop and restart a transfer such as in
	 * the case of reinitiating a stream or retrying an isoc transfer.
	 */
	ret = dwc3_prepare_trbs(dep);
	if (ret < 0)
		return ret;

	starting = !(dep->flags & DWC3_EP_TRANSFER_STARTED);

	/*
	 * If there's no new TRB prepared and we don't need to restart a
	 * transfer, there's no need to update the transfer.
	 */
	if (!ret && !starting)
		return 0;

	req = next_request(&dep->started_list);
	if (!req) {
		dep->flags |= DWC3_EP_PENDING_REQUEST;
		return 0;
	}

	memset(&params, 0, sizeof(params));

	if (starting) {
		params.param0 = upper_32_bits(req->trb_dma);
		params.param1 = lower_32_bits(req->trb_dma);
		cmd = DWC3_DEPCMD_STARTTRANSFER;

		if (dep->stream_capable)
			cmd |= DWC3_DEPCMD_PARAM(req->request.stream_id);

		if (usb_endpoint_xfer_isoc(dep->endpoint.desc))
			cmd |= DWC3_DEPCMD_PARAM(dep->frame_number);
	} else {
		cmd = DWC3_DEPCMD_UPDATETRANSFER |
			DWC3_DEPCMD_PARAM(dep->resource_index);
	}

	ret = dwc3_send_gadget_ep_cmd(dep, cmd, &params);
	if (ret < 0) {
		struct dwc3_request *tmp;

		if (ret == -EAGAIN)
			return ret;

		dwc3_stop_active_transfer(dep, false, true);

		list_for_each_entry_safe(req, tmp, &dep->started_list, list)
			dwc3_gadget_move_cancelled_request(req, DWC3_REQUEST_STATUS_DEQUEUED);

		/* If ep isn't started, then there's no end transfer pending */
		if (!(dep->flags & DWC3_EP_END_TRANSFER_PENDING))
			dwc3_gadget_ep_cleanup_cancelled_requests(dep);

		return ret;
	}

	if (dep->stream_capable && req->request.is_last &&
	    !DWC3_MST_CAPABLE(&dep->dwc->hwparams))
		dep->flags |= DWC3_EP_WAIT_TRANSFER_COMPLETE;

	return 0;
}
/* …（同文件无关代码省略）… */
	return DWC3_DSTS_SOFFN(reg);
}

/**
 * __dwc3_stop_active_transfer - stop the current active transfer
 * @dep: isoc endpoint
/* …（同文件无关代码省略）… */
static int __dwc3_stop_active_transfer(struct dwc3_ep *dep, bool force, bool interrupt)
{
	struct dwc3_gadget_ep_cmd_params params;
	u32 cmd;
	int ret;

	cmd = DWC3_DEPCMD_ENDTRANSFER;
	cmd |= force ? DWC3_DEPCMD_HIPRI_FORCERM : 0;
	cmd |= interrupt ? DWC3_DEPCMD_CMDIOC : 0;
	cmd |= DWC3_DEPCMD_PARAM(dep->resource_index);
	memset(&params, 0, sizeof(params));
	ret = dwc3_send_gadget_ep_cmd(dep, cmd, &params);
	/*
	 * If the End Transfer command was timed out while the device is
	 * not in SETUP phase, it's possible that an incoming Setup packet
	 * may prevent the command's completion. Let's retry when the
	 * ep0state returns to EP0_SETUP_PHASE.
	 */
	if (ret == -ETIMEDOUT && dep->dwc->ep0state != EP0_SETUP_PHASE) {
		dep->flags |= DWC3_EP_DELAY_STOP;
		return 0;
	}

	if (ret)
		dev_err_ratelimited(dep->dwc->dev,
				"end transfer failed: %d\n", ret);

	dep->resource_index = 0;

	if (!interrupt)
		dep->flags &= ~DWC3_EP_TRANSFER_STARTED;
	else if (!ret)
		dep->flags |= DWC3_EP_END_TRANSFER_PENDING;

	dep->flags &= ~DWC3_EP_DELAY_STOP;
	return ret;
}
/* …（同文件无关代码省略）… */
{
	struct dwc3		*dwc = dep->dwc;

	if (!dep->endpoint.desc || !dwc->pullups_connected || !dwc->connected) {
		dev_dbg(dwc->dev, "%s: can't queue to disabled endpoint\n",
				dep->name);
		return -ESHUTDOWN;
/* …（同文件无关代码省略）… */
static void dwc3_gadget_ep_skip_trbs(struct dwc3_ep *dep, struct dwc3_request *req)
{
	int i;

	/* If req->trb is not set, then the request has not started */
	if (!req->trb)
		return;

	/*
	 * If request was already started, this means we had to
	 * stop the transfer. With that we also need to ignore
	 * all TRBs used by the request, however TRBs can only
	 * be modified after completion of END_TRANSFER
	 * command. So what we do here is that we wait for
	 * END_TRANSFER completion and only after that, we jump
	 * over TRBs by clearing HWO and incrementing dequeue
	 * pointer.
	 */
	for (i = 0; i < req->num_trbs; i++) {
		struct dwc3_trb *trb;

		trb = &dep->trb_pool[dep->trb_dequeue];
		trb->ctrl &= ~DWC3_TRB_CTRL_HWO;
		dwc3_ep_inc_deq(dep);
	}

	req->num_trbs = 0;
}
/* …（同文件无关代码省略）… */
static void dwc3_gadget_ep_cleanup_cancelled_requests(struct dwc3_ep *dep)
{
	struct dwc3_request		*req;
	struct dwc3			*dwc = dep->dwc;

	while (!list_empty(&dep->cancelled_list)) {
		req = next_request(&dep->cancelled_list);
		dwc3_gadget_ep_skip_trbs(dep, req);
		switch (req->status) {
		case DWC3_REQUEST_STATUS_DISCONNECTED:
			dwc3_gadget_giveback(dep, req, -ESHUTDOWN);
			break;
		case DWC3_REQUEST_STATUS_DEQUEUED:
			dwc3_gadget_giveback(dep, req, -ECONNRESET);
			break;
		case DWC3_REQUEST_STATUS_STALLED:
			dwc3_gadget_giveback(dep, req, -EPIPE);
			break;
		default:
			dev_err(dwc->dev, "request cancelled with wrong reason:%d\n", req->status);
			dwc3_gadget_giveback(dep, req, -ECONNRESET);
			break;
		}
		/*
		 * The endpoint is disabled, let the dwc3_remove_requests()
		 * handle the cleanup.
		 */
		if (!dep->endpoint.desc)
			break;
	}
}
/* …（同文件无关代码省略）… */
		if (!dep)
			continue;

		dwc3_remove_requests(dwc, dep, -ESHUTDOWN);
	}
}

/* …（同文件无关代码省略）… */
{
	struct dwc3_ep		*dep;
	int			ret = 0;
	u32			reg;

	/*
/* …（同文件无关代码省略）… */
		return ret;
	}

	dep = dwc->eps[0];
	dep->flags = 0;
	ret = __dwc3_gadget_ep_enable(dep, DWC3_DEPCFG_ACTION_INIT);
/* …（同文件无关代码省略）… */
	if (dep->stream_capable)
		dep->flags |= DWC3_EP_IGNORE_NEXT_NOSTREAM;

	dep->flags &= ~DWC3_EP_END_TRANSFER_PENDING;
	dep->flags &= ~DWC3_EP_TRANSFER_STARTED;
	dwc3_gadget_ep_cleanup_cancelled_requests(dep);
/* …（同文件无关代码省略）… */
			dwc3_ep0_send_delayed_status(dwc);
	}

	if ((dep->flags & DWC3_EP_DELAY_START) &&
	    !usb_endpoint_xfer_isoc(dep->endpoint.desc))
		__dwc3_gadget_kick_transfer(dep);

/* …（同文件无关代码省略）… */
void dwc3_stop_active_transfer(struct dwc3_ep *dep, bool force,
	bool interrupt)
{
	struct dwc3 *dwc = dep->dwc;

	/*
	 * Only issue End Transfer command to the control endpoint of a started
	 * Data Phase. Typically we should only do so in error cases such as
	 * invalid/unexpected direction as described in the control transfer
	 * flow of the programming guide.
	 */
	if (dep->number <= 1 && dwc->ep0state != EP0_DATA_PHASE)
		return;

	if (interrupt && (dep->flags & DWC3_EP_DELAY_STOP))
		return;

	if (!(dep->flags & DWC3_EP_TRANSFER_STARTED) ||
	    (dep->flags & DWC3_EP_END_TRANSFER_PENDING))
		return;

	/*
	 * If a Setup packet is received but yet to DMA out, the controller will
	 * not process the End Transfer command of any endpoint. Polling of its
	 * DEPCMD.CmdAct may block setting up TRB for Setup packet, causing a
	 * timeout. Delay issuing the End Transfer command until the Setup TRB is
	 * prepared.
	 */
	if (dwc->ep0state != EP0_SETUP_PHASE && !dwc->delayed_status) {
		dep->flags |= DWC3_EP_DELAY_STOP;
		return;
	}

	/*
	 * NOTICE: We are violating what the Databook says about the
	 * EndTransfer command. Ideally we would _always_ wait for the
	 * EndTransfer Command Completion IRQ, but that's causing too
	 * much trouble synchronizing between us and gadget driver.
	 *
	 * We have discussed this with the IP Provider and it was
	 * suggested to giveback all requests here.
	 *
	 * Note also that a similar handling was tested by Synopsys
	 * (thanks a lot Paul) and nothing bad has come out of it.
	 * In short, what we're doing is issuing EndTransfer with
	 * CMDIOC bit set and delay kicking transfer until the
	 * EndTransfer command had completed.
	 *
	 * As of IP version 3.10a of the DWC_usb3 IP, the controller
	 * supports a mode to work around the above limitation. The
	 * software can poll the CMDACT bit in the DEPCMD register
	 * after issuing a EndTransfer command. This mode is enabled
	 * by writing GUCTL2[14]. This polling is already done in the
	 * dwc3_send_gadget_ep_cmd() function so if the mode is
	 * enabled, the EndTransfer command will have completed upon
	 * returning from this function.
	 *
	 * This mode is NOT available on the DWC_usb31 IP.  In this
	 * case, if the IOC bit is not set, then delay by 1ms
	 * after issuing the EndTransfer command.  This allows for the
	 * controller to handle the command completely before DWC3
	 * remove requests attempts to unmap USB request buffers.
	 */

	__dwc3_stop_active_transfer(dep, force, interrupt);
}
