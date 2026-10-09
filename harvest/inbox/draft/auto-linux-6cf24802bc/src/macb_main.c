// AUTO-DRAFT from torvalds/linux PR #9151d6c42799105e109c8b54dbaad91ce0e17c47
		      skb_is_nonlinear(*skb);
	int padlen = ETH_ZLEN - (*skb)->len;
	int tailroom = skb_tailroom(*skb);
	struct sk_buff *nskb;
	u32 fcs;
  // <<< BUG ANCHOR
/* …（同文件无关代码省略）… */

	if (padlen <= 0) {
		/* FCS could be appended to tailroom. */
		if (!skb_is_nonlinear(*skb) && tailroom >= ETH_FCS_LEN)
			goto add_fcs;
		/* Reallocate with room for the FCS. */
		padlen = ETH_FCS_LEN;
/* …（同文件无关代码省略）… */
		padlen += ETH_FCS_LEN;
	}

	if (cloned || tailroom < padlen) {
		nskb = skb_copy_expand(*skb, 0, padlen, GFP_ATOMIC);
		if (!nskb)
			return -ENOMEM;
