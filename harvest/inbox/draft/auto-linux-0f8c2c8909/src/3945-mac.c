// AUTO-DRAFT from torvalds/linux PR #d24e8ac715de2e16a53c144005b1863660a5fbea

	il_free_channel_map(il);
	il_free_geos(il);
	kfree(il->scan_cmd);
	dev_kfree_skb(il->beacon_skb);
	ieee80211_free_hw(il->hw);
