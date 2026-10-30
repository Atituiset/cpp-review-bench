// AUTO-DRAFT from torvalds/linux PR #4dd16e31135ea8437a720222df34e8745f385d6d
	}
  // <<< BUG ANCHOR
	for (arp_ip_count = 0, i = 0;
	     (arp_ip_count < BOND_MAX_ARP_TARGETS) && arp_ip_target[i]; i++) {
		__be32 ip;

		/* not a complete check, but good enough to catch mistakes */
