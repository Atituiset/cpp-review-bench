// AUTO-DRAFT from torvalds/linux PR #dee3429e94d8089713645706df678ec54a049e13
	int	encap_level;
	__wsum	csum;
	__u16	csum_start;
	/* Number of IPv4/IPv6 GSO handler entries for this packet. */
	u8	recursion_counter;
	/* Max MSS segs per output skb, 0 = no limit */
	u8	max_segs;
