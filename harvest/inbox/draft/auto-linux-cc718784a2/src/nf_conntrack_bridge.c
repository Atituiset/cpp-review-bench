// AUTO-DRAFT from torvalds/linux PR #6785011f8b16abf40b1e9d8b2e332232b3e68822
	int err = 0;
  // <<< BUG ANCHOR
	/* for offloaded checksums cleanup checksum before fragmentation */
	if (skb->ip_summed == CHECKSUM_PARTIAL &&
	    (err = skb_checksum_help(skb)))
		goto blackhole;

	iph = ip_hdr(skb);
