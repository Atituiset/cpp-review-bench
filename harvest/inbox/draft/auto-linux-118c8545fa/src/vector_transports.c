// AUTO-DRAFT from torvalds/linux PR #af32da41b0327b9c6a37856ba82b6760d6c8d10e
	if ((vheader->flags & VIRTIO_NET_HDR_F_DATA_VALID) > 0)
		return 1;

	virtio_net_hdr_to_skb(skb, vheader, virtio_legacy_is_little_endian());
	return 0;
}
