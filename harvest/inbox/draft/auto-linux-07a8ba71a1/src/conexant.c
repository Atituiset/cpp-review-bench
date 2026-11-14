// AUTO-DRAFT from torvalds/linux PR #2800868f63227c8237d70c41e70d441c919620d6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

struct conexant_spec {
	struct hda_gen_spec gen;

	/* extra EAPD pins */
	unsigned int num_eapds;
	hda_nid_t eapds[4];
	bool dynamic_eapd;
	hda_nid_t mute_led_eapd;

	unsigned int parse_flags; /* flag for snd_hda_parse_pin_defcfg() */

	/* OLPC XO specific */
	bool recording;
	bool dc_enable;
	unsigned int dc_input_bias; /* offset into olpc_xo_dc_bias */
	struct nid_path *dc_mode_path;

	int mute_led_polarity;
	unsigned int gpio_led;
	unsigned int gpio_mute_led_mask;
	unsigned int gpio_mic_led_mask;
	bool is_cx11880_sn6140;
};
/* …（同文件无关代码省略）… */
	CXT_FIXUP_HEADPHONE_MIC_PIN,
	CXT_FIXUP_HEADPHONE_MIC,
	CXT_FIXUP_GPIO1,
	CXT_FIXUP_ASPIRE_DMIC,
	CXT_FIXUP_THINKPAD_ACPI,
	CXT_FIXUP_LENOVO_XPAD_ACPI,
/* …（同文件无关代码省略）… */
static void cxt_fixup_stereo_dmic(struct hda_codec *codec,
				  const struct hda_fixup *fix, int action)
{
	struct conexant_spec *spec = codec->spec;
	spec->gen.inv_dmic_split = 1;
}
/* …（同文件无关代码省略）… */
			{ }
		},
	},
	[CXT_FIXUP_ASPIRE_DMIC] = {
		.type = HDA_FIXUP_FUNC,
		.v.func = cxt_fixup_stereo_dmic,
/* …（同文件无关代码省略）… */
	SND_PCI_QUIRK(0x14f1, 0x0252, "MBX-Z60MR100", CXT_FIXUP_HP_A_U),
	SND_PCI_QUIRK(0x14f1, 0x0265, "SWS JS201D", CXT_PINCFG_SWS_JS201D),
	SND_PCI_QUIRK(0x152d, 0x0833, "OLPC XO-1.5", CXT_FIXUP_OLPC_XO),
	SND_PCI_QUIRK(0x17aa, 0x20f2, "Lenovo T400", CXT_PINCFG_LENOVO_TP410),
	SND_PCI_QUIRK(0x17aa, 0x215e, "Lenovo T410", CXT_PINCFG_LENOVO_TP410),
	SND_PCI_QUIRK(0x17aa, 0x215f, "Lenovo T510", CXT_PINCFG_LENOVO_TP410),
