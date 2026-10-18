// AUTO-DRAFT from torvalds/linux PR #a3bb7a0184fbf9fed14aacb9773fe9f6f2097c1b
			0, 0x3f, 0, 0),
	SOC_SINGLE_TLV("Limiter MAX", CS42L56_LIM_THRESHOLD_CTL,
			5, 0x07, 1, alc_tlv),
	SOC_SINGLE_TLV("Limiter Cushion", CS42L56_ALC_THRESHOLD,  // <<< BUG ANCHOR
			2, 0x07, 1, alc_tlv),

	SOC_SINGLE("NG Switch", CS42L56_NOISE_GATE_CTL, 6, 1, 1),
