// AUTO-DRAFT from torvalds/linux PR #a3bb7a0184fbf9fed14aacb9773fe9f6f2097c1b
};
  // <<< BUG ANCHOR
static SOC_ENUM_SINGLE_DECL(ng_type_enum,
			    CS42L52_NOISE_GATE_CTL, 6,
			    cs42l52_ng_type_text);

static const char * const left_swap_text[] = {
