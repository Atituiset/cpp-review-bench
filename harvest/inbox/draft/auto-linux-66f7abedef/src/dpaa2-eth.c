// AUTO-DRAFT from torvalds/linux PR #25faec9ce61c01fb45abf812a222c5b43c3cd5c5
	struct device *dev = priv->net_dev->dev.parent;
	u64 buf_array[DPAA2_ETH_BUFS_PER_CMD];
	struct dpaa2_eth_swa *swa;
	struct page *page;
	dma_addr_t addr;
	int retries = 0;
	int i = 0, err;
	u32 batch;
  // <<< BUG ANCHOR
	/* Allocate buffers visible to WRIOP */
	if (!ch->xsk_zc) {
/* …（同文件无关代码省略）… */
			 * the entire page except for a tailroom reserved for
			 * skb shared info
			 */
			page = dev_alloc_pages(0);
			if (!page)
				goto err_alloc;

			addr = dma_map_page(dev, page, 0, priv->rx_buf_size,
					    DMA_BIDIRECTIONAL);
			if (unlikely(dma_mapping_error(dev, addr)))
				goto err_map;

			buf_array[i] = addr;

/* …（同文件无关代码省略）… */
		 * of DPAA2_ETH_BUFS_PER_CMD. Bail out if the UMEM cannot
		 * provide enough buffers at the moment
		 */
		batch = xsk_buff_alloc_batch(ch->xsk_pool, xdp_buffs,
					     DPAA2_ETH_BUFS_PER_CMD);
		if (!batch)
			goto err_alloc;

/* …（同文件无关代码省略）… */
			swa->xsk.xdp_buff = xdp_buffs[i];

			addr = xsk_buff_xdp_get_frame_dma(xdp_buffs[i]);
			if (unlikely(dma_mapping_error(dev, addr)))
				goto err_map;

			buf_array[i] = addr;

/* …（同文件无关代码省略）… */

	return i;

err_map:
	if (!ch->xsk_zc) {
		__free_pages(page, 0);
	} else {
		for (; i < batch; i++)
			xsk_buff_free(xdp_buffs[i]);
	}
err_alloc:
	/* If we managed to allocate at least some buffers,
	 * release them to hardware
