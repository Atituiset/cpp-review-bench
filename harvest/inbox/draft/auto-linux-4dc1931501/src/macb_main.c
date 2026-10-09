// AUTO-DRAFT from torvalds/linux PR #6b48ed85ae0ca37c7403ef5d512be75975844f2f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static unsigned int macb_adj_dma_desc_idx(struct macb *bp, unsigned int desc_idx)
{
	return desc_idx * (1 + macb_dma64(bp) + macb_dma_ptp(bp));
}
/* …（同文件无关代码省略）… */
static struct macb_dma_desc_64 *macb_64b_desc(struct macb *bp, struct macb_dma_desc *desc)
{
	return (struct macb_dma_desc_64 *)((void *)desc
		+ sizeof(struct macb_dma_desc));
}
/* …（同文件无关代码省略）… */
static unsigned int macb_tx_ring_wrap(struct macb *bp, unsigned int index)
{
	return index & (bp->tx_ring_size - 1);
}
/* …（同文件无关代码省略）… */
static struct macb_dma_desc *macb_tx_desc(struct macb_queue *queue,
					  unsigned int index)
{
	index = macb_tx_ring_wrap(queue->bp, index);
	index = macb_adj_dma_desc_idx(queue->bp, index);
	return &queue->tx_ring[index];
}
/* …（同文件无关代码省略）… */
static struct macb_tx_skb *macb_tx_skb(struct macb_queue *queue,
				       unsigned int index)
{
	return &queue->tx_skb[macb_tx_ring_wrap(queue->bp, index)];
}
/* …（同文件无关代码省略）… */
static void macb_tx_unmap(struct macb *bp, struct macb_tx_skb *tx_skb, int budget)
{
	if (tx_skb->mapping) {
		if (tx_skb->mapped_as_page)
			dma_unmap_page(&bp->pdev->dev, tx_skb->mapping,
				       tx_skb->size, DMA_TO_DEVICE);
		else
			dma_unmap_single(&bp->pdev->dev, tx_skb->mapping,
					 tx_skb->size, DMA_TO_DEVICE);
		tx_skb->mapping = 0;
	}

	if (tx_skb->skb) {
		dev_consume_skb_any(tx_skb->skb);
		tx_skb->skb = NULL;
	}
}
/* …（同文件无关代码省略）… */
static void macb_set_addr(struct macb *bp, struct macb_dma_desc *desc, dma_addr_t addr)
{
	if (macb_dma64(bp)) {
		struct macb_dma_desc_64 *desc_64;

		desc_64 = macb_64b_desc(bp, desc);
		desc_64->addrh = upper_32_bits(addr);
		/* The low bits of RX address contain the RX_USED bit, clearing
		 * of which allows packet RX. Make sure the high bits are also
		 * visible to HW at that point.
		 */
		dma_wmb();
	}

	desc->addr = lower_32_bits(addr);
}
/* …（同文件无关代码省略）… */
static bool ptp_one_step_sync(struct sk_buff *skb)
{
	struct ptp_header *hdr;
	unsigned int ptp_class;
	u8 msgtype;

	/* No need to parse packet if PTP TS is not involved */
	if (likely(!(skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP)))
		goto not_oss;

	/* Identify and return whether PTP one step sync is being processed */
	ptp_class = ptp_classify_raw(skb);
	if (ptp_class == PTP_CLASS_NONE)
		goto not_oss;

	hdr = ptp_parse_header(skb, ptp_class);
	if (!hdr)
		goto not_oss;

	if (hdr->flag_field[0] & PTP_FLAG_TWOSTEP)
		goto not_oss;

	msgtype = ptp_get_msgtype(hdr, ptp_class);
	if (msgtype == PTP_MSGTYPE_SYNC)
		return true;

not_oss:
	return false;
}
/* …（同文件无关代码省略）… */
static unsigned int macb_tx_map(struct macb *bp,
				struct macb_queue *queue,
				struct sk_buff *skb,
				unsigned int hdrlen,
				u8 fcs_len)
{
	unsigned int f, nr_frags = skb_shinfo(skb)->nr_frags;
	unsigned int len, i, tx_head = queue->tx_head;
	u32 ctrl, lso_ctrl = 0, seq_ctrl = 0;
	unsigned int eof = 1, mss_mfs = 0;
	struct macb_tx_skb *tx_skb = NULL;
	struct macb_dma_desc *desc;
	unsigned int offset, size;
	dma_addr_t mapping;

	/* LSO */
	if (skb_shinfo(skb)->gso_size != 0) {
		if (ip_hdr(skb)->protocol == IPPROTO_UDP)
			/* UDP - UFO */
			lso_ctrl = MACB_LSO_UFO_ENABLE;
		else
			/* TCP - TSO */
			lso_ctrl = MACB_LSO_TSO_ENABLE;
	}

	/* First, map non-paged data */
	len = skb_headlen(skb);

	/* first buffer length */
	size = hdrlen;

	offset = 0;
	while (len) {
		tx_skb = macb_tx_skb(queue, tx_head);

		mapping = dma_map_single(&bp->pdev->dev,
					 skb->data + offset,
					 size, DMA_TO_DEVICE);
		if (dma_mapping_error(&bp->pdev->dev, mapping))
			goto dma_error;

		/* Save info to properly release resources */
		tx_skb->skb = NULL;
		tx_skb->mapping = mapping;
		tx_skb->size = size;
		tx_skb->mapped_as_page = false;

		len -= size;
		offset += size;
		tx_head++;

		size = umin(len, bp->max_tx_length);
	}

	/* Then, map paged data from fragments */
	for (f = 0; f < nr_frags; f++) {
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[f];

		len = skb_frag_size(frag);
		offset = 0;
		while (len) {
			size = umin(len, bp->max_tx_length);
			tx_skb = macb_tx_skb(queue, tx_head);

			mapping = skb_frag_dma_map(&bp->pdev->dev, frag,
						   offset, size, DMA_TO_DEVICE);
			if (dma_mapping_error(&bp->pdev->dev, mapping))
				goto dma_error;

			/* Save info to properly release resources */
			tx_skb->skb = NULL;
			tx_skb->mapping = mapping;
			tx_skb->size = size;
			tx_skb->mapped_as_page = true;

			len -= size;
			offset += size;
			tx_head++;
		}
	}

	/* Should never happen */
	if (unlikely(!tx_skb)) {
		netdev_err(bp->netdev, "BUG! empty skb!\n");
		return 0;
	}

	/* This is the last buffer of the frame: save socket buffer */
	tx_skb->skb = skb;
	tx_skb->fcs_len = fcs_len;

	/* Update TX ring: update buffer descriptors in reverse order
	 * to avoid race condition
	 */

	/* Set 'TX_USED' bit in buffer descriptor at tx_head position
	 * to set the end of TX queue
	 */
	i = tx_head;
	ctrl = MACB_BIT(TX_USED);
	desc = macb_tx_desc(queue, i);
	desc->ctrl = ctrl;

	if (lso_ctrl) {
		if (lso_ctrl == MACB_LSO_UFO_ENABLE)
			/* include header and FCS in value given to h/w */
			mss_mfs = skb_shinfo(skb)->gso_size +
					skb_transport_offset(skb) +
					ETH_FCS_LEN;
		else /* TSO */ {
			mss_mfs = skb_shinfo(skb)->gso_size;
			/* TCP Sequence Number Source Select
			 * can be set only for TSO
			 */
			seq_ctrl = 0;
		}
	}

	do {
		i--;
		tx_skb = macb_tx_skb(queue, i);
		desc = macb_tx_desc(queue, i);

		ctrl = (u32)tx_skb->size;
		if (eof) {
			ctrl |= MACB_BIT(TX_LAST);
			eof = 0;
		}
		if (unlikely(macb_tx_ring_wrap(bp, i) == bp->tx_ring_size - 1))
			ctrl |= MACB_BIT(TX_WRAP);

		/* First descriptor is header descriptor */
		if (i == queue->tx_head) {
			ctrl |= MACB_BF(TX_LSO, lso_ctrl);
			ctrl |= MACB_BF(TX_TCP_SEQ_SRC, seq_ctrl);
			if ((bp->netdev->features & NETIF_F_HW_CSUM) &&
			    skb->ip_summed != CHECKSUM_PARTIAL && !lso_ctrl &&
			    !ptp_one_step_sync(skb))
				ctrl |= MACB_BIT(TX_NOCRC);
		} else
			/* Only set MSS/MFS on payload descriptors
			 * (second or later descriptor)
			 */
			ctrl |= MACB_BF(MSS_MFS, mss_mfs);

		/* Set TX buffer descriptor */
		macb_set_addr(bp, desc, tx_skb->mapping);
		/* desc->addr must be visible to hardware before clearing
		 * 'TX_USED' bit in desc->ctrl.
		 */
		wmb();
		desc->ctrl = ctrl;
	} while (i != queue->tx_head);

	queue->tx_head = tx_head;

	return 0;

dma_error:
	netdev_err(bp->netdev, "TX DMA map failed\n");

	for (i = queue->tx_head; i != tx_head; i++) {
		tx_skb = macb_tx_skb(queue, i);

		macb_tx_unmap(bp, tx_skb, 0);
	}

	return -ENOMEM;
}
/* …（同文件无关代码省略）… */
static inline int macb_clear_csum(struct sk_buff *skb)
{
	/* no change for packets without checksum offloading */
	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	/* make sure we can modify the header */
	if (unlikely(skb_cow_head(skb, 0)))
		return -1;

	/* initialize checksum field
	 * This is required - at least for Zynq, which otherwise calculates
	 * wrong UDP header checksums for UDP packets with UDP data len <=2
	 */
	*(__sum16 *)(skb_checksum_start(skb) + skb->csum_offset) = 0;
	return 0;
}

/* Returns a negative errno, or the FCS bytes appended (0 or ETH_FCS_LEN). */
static int macb_pad_and_fcs(struct sk_buff **skb, struct net_device *netdev)
{
	bool cloned = skb_cloned(*skb) || skb_header_cloned(*skb) ||
		      skb_is_nonlinear(*skb);
/* …（同文件无关代码省略）… */
	struct sk_buff *nskb;
	u32 fcs;

	if (!(netdev->features & NETIF_F_HW_CSUM) ||
	    !((*skb)->ip_summed != CHECKSUM_PARTIAL) ||
	    skb_shinfo(*skb)->gso_size || ptp_one_step_sync(*skb))
		return 0;

	if (padlen <= 0) {
		/* FCS could be appeded to tailroom. */
		if (tailroom >= ETH_FCS_LEN)
			goto add_fcs;
		/* No room for FCS, need to reallocate skb. */
		else
			padlen = ETH_FCS_LEN;
	} else {
		/* Add room for FCS. */
		padlen += ETH_FCS_LEN;
/* …（同文件无关代码省略）… */
	unsigned int desc_cnt, nr_frags, frag_size, f;
	struct macb_queue *queue = &bp->queues[q];
	netdev_tx_t ret = NETDEV_TX_OK;
	unsigned int hdrlen;
	unsigned long flags;
	int fcs_len;
	bool is_lso;

	if (macb_clear_csum(skb)) {
		dev_kfree_skb_any(skb);
		return ret;
	}

	fcs_len = macb_pad_and_fcs(&skb, netdev);
	if (fcs_len < 0) {
		dev_kfree_skb_any(skb);
		return ret;
	}

	if (macb_dma_ptp(bp) &&
	    (skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP))
		skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;

	is_lso = (skb_shinfo(skb)->gso_size != 0);

	if (is_lso) {
		/* length of headers */
/* …（同文件无关代码省略）… */
			/* if this is required, would need to copy to single buffer */
			return NETDEV_TX_BUSY;
		}
	} else
		hdrlen = umin(skb_headlen(skb), bp->max_tx_length);

#if defined(DEBUG) && defined(VERBOSE_DEBUG)
	netdev_vdbg(bp->netdev,
/* …（同文件无关代码省略）… */
	 * socket buffer: skb fragments of jumbo frames may need to be
	 * split into many buffer descriptors.
	 */
	if (is_lso && (skb_headlen(skb) > hdrlen))
		/* extra header descriptor if also payload in first buffer */
		desc_cnt = DIV_ROUND_UP((skb_headlen(skb) - hdrlen), bp->max_tx_length) + 1;
	else
		desc_cnt = DIV_ROUND_UP(skb_headlen(skb), bp->max_tx_length);
	nr_frags = skb_shinfo(skb)->nr_frags;
	for (f = 0; f < nr_frags; f++) {
		frag_size = skb_frag_size(&skb_shinfo(skb)->frags[f]);
		desc_cnt += DIV_ROUND_UP(frag_size, bp->max_tx_length);
/* …（同文件无关代码省略）… */
		goto unlock;
	}

	/* Map socket buffer for DMA transfer */
	if (macb_tx_map(bp, queue, skb, hdrlen, fcs_len)) {
		dev_kfree_skb_any(skb);
