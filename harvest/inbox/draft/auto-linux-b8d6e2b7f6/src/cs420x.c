// AUTO-DRAFT from torvalds/linux PR #2800868f63227c8237d70c41e70d441c919620d6
  // <<< BUG ANCHOR
static const struct hda_quirk cs4208_fixup_tbl[] = {
	SND_PCI_QUIRK_VENDOR(0x106b, "Apple", CS4208_MAC_AUTO),
	SND_PCI_QUIRK(0x8086, 0x7270, "MacBookAir 7,2", CS4208_MAC_AUTO),
	{} /* terminator */
};

/* …（同文件无关代码省略）… */
	SND_PCI_QUIRK(0x106b, 0x7800, "MacPro 6,1", CS4208_MACMINI),
	SND_PCI_QUIRK(0x106b, 0x7b00, "MacBookPro 12,1", CS4208_MBP11),
	SND_PCI_QUIRK(0x106b, 0x7f00, "iMac 16,1", CS4208_MBP11),
	SND_PCI_QUIRK(0x8086, 0x7270, "MacBookAir 7,2", CS4208_MBA6),
	{} /* terminator */
};
