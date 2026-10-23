// AUTO-DRAFT from torvalds/linux PR #864c07de0c7dcbca58b61625dfa2e461ae63b242
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
struct cc_param {
	u32 rp_4p7k_code;
	u32 rp_36k_code;
	u32 rp_12k_code;
	u32 rd_code;
	u32 ra_code;
	u32 vref_2p6v;
	u32 vref_1p23v;
	u32 vref_0p8v;
	u32 vref_0p66v;
	u32 vref_0p4v;
	u32 vref_0p2v;
	u32 vref_1_1p6v;
	u32 vref_0_1p6v;
};
/* …（同文件无关代码省略）… */
struct type_c_cfg {
	int parameter_ver; /* Parameter version */
	int cc_dfp_mode;
	struct cc_param cc1_param;
	struct cc_param cc2_param;

	u32 debounce_val;
	bool use_defalut_parameter;
};
/* …（同文件无关代码省略）… */
		goto err;
	}

	type_c->type_c_cfg = devm_kzalloc(dev, sizeof(*type_c_cfg), GFP_KERNEL);
	if (!type_c->type_c_cfg)
		return -ENOMEM;
