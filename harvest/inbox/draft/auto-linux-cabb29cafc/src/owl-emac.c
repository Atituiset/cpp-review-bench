// AUTO-DRAFT from torvalds/linux PR #045958acaa75205b31578cb22c4345e28dd2e4bf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static u32 owl_emac_reg_read(struct owl_emac_priv *priv, u32 reg)
{
	return readl(priv->base + reg);
}
/* …（同文件无关代码省略）… */
static void owl_emac_reg_write(struct owl_emac_priv *priv, u32 reg, u32 data)
{
	writel(data, priv->base + reg);
}
/* …（同文件无关代码省略）… */
static u32 owl_emac_reg_update(struct owl_emac_priv *priv,
			       u32 reg, u32 mask, u32 val)
{
	u32 data, old_val;

	data = owl_emac_reg_read(priv, reg);
	old_val = data & mask;

	data &= ~mask;
	data |= val & mask;

	owl_emac_reg_write(priv, reg, data);

	return old_val;
}
/* …（同文件无关代码省略）… */
static struct device *owl_emac_get_dev(struct owl_emac_priv *priv)
{
	return priv->netdev->dev.parent;
}
/* …（同文件无关代码省略）… */
static void owl_emac_irq_enable(struct owl_emac_priv *priv)
{
	/* Enable all interrupts except TU.
	 *
	 * Note the NIE and AIE bits shall also be set in order to actually
	 * enable the selected interrupts.
	 */
	owl_emac_reg_write(priv, OWL_EMAC_REG_MAC_CSR7,
			   OWL_EMAC_BIT_MAC_CSR7_NIE |
			   OWL_EMAC_BIT_MAC_CSR7_AIE |
			   OWL_EMAC_BIT_MAC_CSR7_ALL_NOT_TUE);
}
/* …（同文件无关代码省略）… */
static u32 owl_emac_irq_status(struct owl_emac_priv *priv)
{
	return owl_emac_reg_read(priv, OWL_EMAC_REG_MAC_CSR5);
}
/* …（同文件无关代码省略）… */
static u32 owl_emac_irq_clear(struct owl_emac_priv *priv)
{
	u32 val = owl_emac_irq_status(priv);

	owl_emac_reg_write(priv, OWL_EMAC_REG_MAC_CSR5, val);

	return val;
}
/* …（同文件无关代码省略）… */
static dma_addr_t owl_emac_dma_map_rx(struct owl_emac_priv *priv,
				      struct sk_buff *skb)
{
	struct device *dev = owl_emac_get_dev(priv);

	/* Buffer pointer for the RX DMA descriptor must be word aligned. */
	return dma_map_single(dev, skb_tail_pointer(skb),
			      skb_tailroom(skb), DMA_FROM_DEVICE);
}
/* …（同文件无关代码省略）… */
static void owl_emac_dma_unmap_rx(struct owl_emac_priv *priv,
				  struct sk_buff *skb, dma_addr_t dma_addr)
{
	struct device *dev = owl_emac_get_dev(priv);

	dma_unmap_single(dev, dma_addr, skb_tailroom(skb), DMA_FROM_DEVICE);
}
/* …（同文件无关代码省略）… */
static void owl_emac_dma_unmap_tx(struct owl_emac_priv *priv,
				  struct sk_buff *skb, dma_addr_t dma_addr)
{
	struct device *dev = owl_emac_get_dev(priv);

	dma_unmap_single(dev, dma_addr, skb_headlen(skb), DMA_TO_DEVICE);
}
/* …（同文件无关代码省略）… */
static unsigned int owl_emac_ring_num_unused(struct owl_emac_ring *ring)
{
	return CIRC_SPACE(ring->head, ring->tail, ring->size);
}
/* …（同文件无关代码省略）… */
static unsigned int owl_emac_ring_get_next(struct owl_emac_ring *ring,
					   unsigned int cur)
{
	return (cur + 1) & (ring->size - 1);
}
/* …（同文件无关代码省略）… */
static void owl_emac_ring_push_head(struct owl_emac_ring *ring)
{
	ring->head = owl_emac_ring_get_next(ring, ring->head);
}
/* …（同文件无关代码省略）… */
static void owl_emac_ring_pop_tail(struct owl_emac_ring *ring)
{
	ring->tail = owl_emac_ring_get_next(ring, ring->tail);
}
/* …（同文件无关代码省略）… */
static struct sk_buff *owl_emac_alloc_skb(struct net_device *netdev)
{
	struct sk_buff *skb;
	int offset;

	skb = netdev_alloc_skb(netdev, OWL_EMAC_RX_FRAME_MAX_LEN +
			       OWL_EMAC_SKB_RESERVE);
	if (unlikely(!skb))
		return NULL;

	/* Ensure 4 bytes DMA alignment. */
	offset = ((uintptr_t)skb->data) & (OWL_EMAC_SKB_ALIGN - 1);
	if (unlikely(offset))
		skb_reserve(skb, OWL_EMAC_SKB_ALIGN - offset);

	return skb;
}
/* …（同文件无关代码省略）… */
static void owl_emac_dma_cmd_resume_rx(struct owl_emac_priv *priv)
{
	owl_emac_reg_write(priv, OWL_EMAC_REG_MAC_CSR2,
			   OWL_EMAC_VAL_MAC_CSR2_RPD);
}
/* …（同文件无关代码省略）… */
static u32 owl_emac_dma_cmd_set(struct owl_emac_priv *priv, u32 status)
{
	return owl_emac_reg_update(priv, OWL_EMAC_REG_MAC_CSR6,
				   OWL_EMAC_MSK_MAC_CSR6_STSR, status);
}
/* …（同文件无关代码省略）… */
static u32 owl_emac_dma_cmd_stop(struct owl_emac_priv *priv)
{
	return owl_emac_dma_cmd_set(priv, 0);
}
/* …（同文件无关代码省略）… */
static void owl_emac_update_link_state(struct owl_emac_priv *priv)
{
	u32 val, status;

	if (priv->pause) {
		val = OWL_EMAC_BIT_MAC_CSR20_FCE | OWL_EMAC_BIT_MAC_CSR20_TUE;
		val |= OWL_EMAC_BIT_MAC_CSR20_TPE | OWL_EMAC_BIT_MAC_CSR20_RPE;
		val |= OWL_EMAC_BIT_MAC_CSR20_BPE;
	} else {
		val = 0;
	}

	/* Update flow control. */
	owl_emac_reg_write(priv, OWL_EMAC_REG_MAC_CSR20, val);

	val = (priv->speed == SPEED_100) ? OWL_EMAC_VAL_MAC_CSR6_SPEED_100M :
					   OWL_EMAC_VAL_MAC_CSR6_SPEED_10M;
	val <<= OWL_EMAC_OFF_MAC_CSR6_SPEED;

	if (priv->duplex == DUPLEX_FULL)
		val |= OWL_EMAC_BIT_MAC_CSR6_FD;

	spin_lock_bh(&priv->lock);

	/* Temporarily stop DMA TX & RX. */
	status = owl_emac_dma_cmd_stop(priv);

	/* Update operation modes. */
	owl_emac_reg_update(priv, OWL_EMAC_REG_MAC_CSR6,
			    OWL_EMAC_MSK_MAC_CSR6_SPEED |
			    OWL_EMAC_BIT_MAC_CSR6_FD, val);

	/* Restore DMA TX & RX status. */
	owl_emac_dma_cmd_set(priv, status);

	spin_unlock_bh(&priv->lock);
}
/* …（同文件无关代码省略）… */
static void owl_emac_adjust_link(struct net_device *netdev)
{
	struct owl_emac_priv *priv = netdev_priv(netdev);
	struct phy_device *phydev = netdev->phydev;
	bool state_changed = false;

	if (phydev->link) {
		if (!priv->link) {
			priv->link = phydev->link;
			state_changed = true;
		}

		if (priv->speed != phydev->speed) {
			priv->speed = phydev->speed;
			state_changed = true;
		}

		if (priv->duplex != phydev->duplex) {
			priv->duplex = phydev->duplex;
			state_changed = true;
		}

		if (priv->pause != phydev->pause) {
			priv->pause = phydev->pause;
			state_changed = true;
		}
	} else {
		if (priv->link) {
			priv->link = phydev->link;
			state_changed = true;
		}
	}

	if (state_changed) {
		if (phydev->link)
			owl_emac_update_link_state(priv);

		if (netif_msg_link(priv))
			phy_print_status(phydev);
	}
}
/* …（同文件无关代码省略）… */
static bool owl_emac_tx_complete_tail(struct owl_emac_priv *priv)
{
	struct owl_emac_ring *ring = &priv->tx_ring;
	struct net_device *netdev = priv->netdev;
	struct owl_emac_ring_desc *desc;
	struct sk_buff *skb;
	unsigned int tx_tail;
	u32 status;

	tx_tail = ring->tail;
	desc = &ring->descs[tx_tail];

	status = READ_ONCE(desc->status);
	dma_rmb(); /* Ensure data has been read before used. */

	if (status & OWL_EMAC_BIT_TDES0_OWN)
		return false;

	/* Check for errors. */
	if (status & OWL_EMAC_BIT_TDES0_ES) {
		dev_dbg_ratelimited(&netdev->dev,
				    "TX complete error status: 0x%08x\n",
				    status);

		netdev->stats.tx_errors++;

		if (status & OWL_EMAC_BIT_TDES0_UF)
			netdev->stats.tx_fifo_errors++;

		if (status & OWL_EMAC_BIT_TDES0_EC)
			netdev->stats.tx_aborted_errors++;

		if (status & OWL_EMAC_BIT_TDES0_LC)
			netdev->stats.tx_window_errors++;

		if (status & OWL_EMAC_BIT_TDES0_NC)
			netdev->stats.tx_heartbeat_errors++;

		if (status & OWL_EMAC_BIT_TDES0_LO)
			netdev->stats.tx_carrier_errors++;
	} else {
		netdev->stats.tx_packets++;
		netdev->stats.tx_bytes += ring->skbs[tx_tail]->len;
	}

	/* Some collisions occurred, but pkt has been transmitted. */
	if (status & OWL_EMAC_BIT_TDES0_DE)
		netdev->stats.collisions++;

	skb = ring->skbs[tx_tail];
	owl_emac_dma_unmap_tx(priv, skb, ring->skbs_dma[tx_tail]);
	dev_kfree_skb(skb);

	ring->skbs[tx_tail] = NULL;
	ring->skbs_dma[tx_tail] = 0;

	owl_emac_ring_pop_tail(ring);

	if (unlikely(netif_queue_stopped(netdev)))
		netif_wake_queue(netdev);

	return true;
}
/* …（同文件无关代码省略）… */
static void owl_emac_tx_complete(struct owl_emac_priv *priv)
{
	struct owl_emac_ring *ring = &priv->tx_ring;
	struct net_device *netdev = priv->netdev;
	unsigned int tx_next;
	u32 status;

	spin_lock(&priv->lock);

	while (ring->tail != ring->head) {
		if (!owl_emac_tx_complete_tail(priv))
			break;
	}

	/* FIXME: This is a workaround for a MAC hardware bug not clearing
	 * (sometimes) the OWN bit for a transmitted frame descriptor.
	 *
	 * At this point, when TX queue is full, the tail descriptor has the
	 * OWN bit set, which normally means the frame has not been processed
	 * or transmitted yet. But if there is at least one descriptor in the
	 * queue having the OWN bit cleared, we can safely assume the tail
	 * frame has been also processed by the MAC hardware.
	 *
	 * If that's the case, let's force the frame completion by manually
	 * clearing the OWN bit.
	 */
	if (unlikely(!owl_emac_ring_num_unused(ring))) {
		tx_next = ring->tail;

		while ((tx_next = owl_emac_ring_get_next(ring, tx_next)) != ring->head) {
			status = READ_ONCE(ring->descs[tx_next].status);
			dma_rmb(); /* Ensure data has been read before used. */

			if (status & OWL_EMAC_BIT_TDES0_OWN)
				continue;

			netdev_dbg(netdev, "Found uncleared TX desc OWN bit\n");

			status = READ_ONCE(ring->descs[ring->tail].status);
			dma_rmb(); /* Ensure data has been read before used. */
			status &= ~OWL_EMAC_BIT_TDES0_OWN;
			WRITE_ONCE(ring->descs[ring->tail].status, status);

			owl_emac_tx_complete_tail(priv);
			break;
		}
	}

	spin_unlock(&priv->lock);
}
/* …（同文件无关代码省略）… */
static int owl_emac_rx_process(struct owl_emac_priv *priv, int budget)
{
	struct owl_emac_ring *ring = &priv->rx_ring;
	struct device *dev = owl_emac_get_dev(priv);
	struct net_device *netdev = priv->netdev;
	struct owl_emac_ring_desc *desc;
	struct sk_buff *curr_skb, *new_skb;
	dma_addr_t curr_dma, new_dma;
	unsigned int rx_tail, len;
	u32 status;
	int recv = 0;

	while (recv < budget) {
		spin_lock(&priv->lock);

		rx_tail = ring->tail;
		desc = &ring->descs[rx_tail];

		status = READ_ONCE(desc->status);
		dma_rmb(); /* Ensure data has been read before used. */

		if (status & OWL_EMAC_BIT_RDES0_OWN) {
			spin_unlock(&priv->lock);
			break;
		}

		curr_skb = ring->skbs[rx_tail];
		curr_dma = ring->skbs_dma[rx_tail];
		owl_emac_ring_pop_tail(ring);

		spin_unlock(&priv->lock);

		if (status & (OWL_EMAC_BIT_RDES0_DE | OWL_EMAC_BIT_RDES0_RF |
		    OWL_EMAC_BIT_RDES0_TL | OWL_EMAC_BIT_RDES0_CS |
		    OWL_EMAC_BIT_RDES0_DB | OWL_EMAC_BIT_RDES0_CE |
		    OWL_EMAC_BIT_RDES0_ZERO)) {
			dev_dbg_ratelimited(&netdev->dev,
					    "RX desc error status: 0x%08x\n",
					    status);

			if (status & OWL_EMAC_BIT_RDES0_DE)
				netdev->stats.rx_over_errors++;

			if (status & (OWL_EMAC_BIT_RDES0_RF | OWL_EMAC_BIT_RDES0_DB))
				netdev->stats.rx_frame_errors++;

			if (status & OWL_EMAC_BIT_RDES0_TL)
				netdev->stats.rx_length_errors++;

			if (status & OWL_EMAC_BIT_RDES0_CS)
				netdev->stats.collisions++;

			if (status & OWL_EMAC_BIT_RDES0_CE)
				netdev->stats.rx_crc_errors++;

			if (status & OWL_EMAC_BIT_RDES0_ZERO)
				netdev->stats.rx_fifo_errors++;

			goto drop_skb;
		}

		len = (status & OWL_EMAC_MSK_RDES0_FL) >> OWL_EMAC_OFF_RDES0_FL;
		if (unlikely(len > OWL_EMAC_RX_FRAME_MAX_LEN)) {
			netdev->stats.rx_length_errors++;
			netdev_err(netdev, "invalid RX frame len: %u\n", len);
			goto drop_skb;
		}

		/* Prepare new skb before receiving the current one. */
		new_skb = owl_emac_alloc_skb(netdev);
		if (unlikely(!new_skb))
			goto drop_skb;

		new_dma = owl_emac_dma_map_rx(priv, new_skb);
		if (dma_mapping_error(dev, new_dma)) {
			dev_kfree_skb(new_skb);
			netdev_err(netdev, "RX DMA mapping failed\n");
			goto drop_skb;
		}

		owl_emac_dma_unmap_rx(priv, curr_skb, curr_dma);

		skb_put(curr_skb, len - ETH_FCS_LEN);
		curr_skb->ip_summed = CHECKSUM_NONE;
		curr_skb->protocol = eth_type_trans(curr_skb, netdev);
		curr_skb->dev = netdev;

		netif_receive_skb(curr_skb);

		netdev->stats.rx_packets++;
		netdev->stats.rx_bytes += len;
		recv++;
		goto push_skb;

drop_skb:
		netdev->stats.rx_dropped++;
		netdev->stats.rx_errors++;
		/* Reuse the current skb. */
		new_skb = curr_skb;
		new_dma = curr_dma;

push_skb:
		spin_lock(&priv->lock);

		ring->skbs[ring->head] = new_skb;
		ring->skbs_dma[ring->head] = new_dma;

		WRITE_ONCE(desc->buf_addr, new_dma);
		dma_wmb(); /* Flush descriptor before changing ownership. */
		WRITE_ONCE(desc->status, OWL_EMAC_BIT_RDES0_OWN);

		owl_emac_ring_push_head(ring);

		spin_unlock(&priv->lock);
	}

	return recv;
}
/* …（同文件无关代码省略）… */
static int owl_emac_poll(struct napi_struct *napi, int budget)
{
	int work_done = 0, ru_cnt = 0, recv;
	static int tx_err_cnt, rx_err_cnt;
	struct owl_emac_priv *priv;
	u32 status, proc_status;

	priv = container_of(napi, struct owl_emac_priv, napi);

	while ((status = owl_emac_irq_clear(priv)) &
	       (OWL_EMAC_BIT_MAC_CSR5_NIS | OWL_EMAC_BIT_MAC_CSR5_AIS)) {
		recv = 0;

		/* TX setup frame raises ETI instead of TI. */
		if (status & (OWL_EMAC_BIT_MAC_CSR5_TI | OWL_EMAC_BIT_MAC_CSR5_ETI)) {
			owl_emac_tx_complete(priv);
			tx_err_cnt = 0;

			/* Count MAC internal RX errors. */
			proc_status = status & OWL_EMAC_MSK_MAC_CSR5_RS;
			proc_status >>= OWL_EMAC_OFF_MAC_CSR5_RS;
			if (proc_status == OWL_EMAC_VAL_MAC_CSR5_RS_DATA ||
			    proc_status == OWL_EMAC_VAL_MAC_CSR5_RS_CDES ||
			    proc_status == OWL_EMAC_VAL_MAC_CSR5_RS_FDES)
				rx_err_cnt++;
		}

		if (status & OWL_EMAC_BIT_MAC_CSR5_RI) {
			recv = owl_emac_rx_process(priv, budget - work_done);
			rx_err_cnt = 0;

			/* Count MAC internal TX errors. */
			proc_status = status & OWL_EMAC_MSK_MAC_CSR5_TS;
			proc_status >>= OWL_EMAC_OFF_MAC_CSR5_TS;
			if (proc_status == OWL_EMAC_VAL_MAC_CSR5_TS_DATA ||
			    proc_status == OWL_EMAC_VAL_MAC_CSR5_TS_CDES)
				tx_err_cnt++;
		} else if (status & OWL_EMAC_BIT_MAC_CSR5_RU) {
			/* MAC AHB is in suspended state, will return to RX
			 * descriptor processing when the host changes ownership
			 * of the descriptor and either an RX poll demand CMD is
			 * issued or a new frame is recognized by the MAC AHB.
			 */
			if (++ru_cnt == 2)
				owl_emac_dma_cmd_resume_rx(priv);

			recv = owl_emac_rx_process(priv, budget - work_done);

			/* Guard against too many RU interrupts. */
			if (ru_cnt > 3)
				break;
		}

		work_done += recv;
		if (work_done >= budget)
			break;
	}

	if (work_done < budget) {
		napi_complete_done(napi, work_done);
		owl_emac_irq_enable(priv);
	}

	/* Reset MAC when getting too many internal TX or RX errors. */
	if (tx_err_cnt > 10 || rx_err_cnt > 10) {
		netdev_dbg(priv->netdev, "%s error status: 0x%08x\n",
			   tx_err_cnt > 10 ? "TX" : "RX", status);
		rx_err_cnt = 0;
		tx_err_cnt = 0;
		schedule_work(&priv->mac_reset_task);
	}

	return work_done;
}
/* …（同文件无关代码省略）… */
static int owl_emac_phy_init(struct net_device *netdev)
{
	struct owl_emac_priv *priv = netdev_priv(netdev);
	struct device *dev = owl_emac_get_dev(priv);
	struct phy_device *phy;

	phy = of_phy_get_and_connect(netdev, dev->of_node,
				     owl_emac_adjust_link);
	if (!phy)
		return -ENODEV;

	phy_set_sym_pause(phy, true, true, true);

	if (netif_msg_link(priv))
		phy_attached_info(phy);

	return 0;
}
/* …（同文件无关代码省略）… */
		return ret;
	}

	ret = owl_emac_phy_init(netdev);
	if (ret) {
		dev_err(dev, "failed to initialize PHY\n");
/* …（同文件无关代码省略）… */
	netdev->min_mtu = OWL_EMAC_MTU_MIN;
	netdev->max_mtu = OWL_EMAC_MTU_MAX;
	netdev->watchdog_timeo = OWL_EMAC_TX_TIMEOUT;
	netdev->netdev_ops = &owl_emac_netdev_ops;
	netdev->ethtool_ops = &owl_emac_ethtool_ops;
	netif_napi_add(netdev, &priv->napi, owl_emac_poll);
