// AUTO-DRAFT from torvalds/linux PR #5906f156e8818d29839d7d7280b691f627b59883
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct tps6594_regulator_irq_type {
	const char *irq_name;
	const char *regulator_name;
	const char *event_name;
	unsigned long event;
};
/* …（同文件无关代码省略）… */
#define TPS6594_REGULATOR(_name, _of, _id, _type, _ops, _n, _vr, _vm, _er, \
			   _em, _cr, _cm, _lr, _nlr, _delay, _fuv, \
			   _ct, _ncl, _bpm) \
	{								\
		.name			= _name,			\
		.of_match		= _of,				\
		.regulators_node	= of_match_ptr("regulators"),	\
		.supply_name		= _of,				\
		.id			= _id,				\
		.ops			= &(_ops),			\
		.n_voltages		= _n,				\
		.type			= _type,			\
		.owner			= THIS_MODULE,			\
		.vsel_reg		= _vr,				\
		.vsel_mask		= _vm,				\
		.csel_reg		= _cr,				\
		.csel_mask		= _cm,				\
		.curr_table		= _ct,				\
		.n_current_limits	= _ncl,				\
		.enable_reg		= _er,				\
		.enable_mask		= _em,				\
		.volt_table		= NULL,				\
		.linear_ranges		= _lr,				\
		.n_linear_ranges	= _nlr,				\
		.ramp_delay		= _delay,			\
		.fixed_uV		= _fuv,				\
		.bypass_reg		= _vr,				\
		.bypass_mask		= _bpm,				\
	}								\

/* …（同文件无关代码省略）… */
			  TPS65224_MASK_BUCKS_VSET,
			  TPS6594_REG_BUCKX_CTRL(1),
			  TPS6594_BIT_BUCK_EN, 0, 0, tps65224_bucks_2_3_4_ranges,
			  4, 0, 0, NULL, 0, 0),
	TPS6594_REGULATOR("BUCK3", "buck3", TPS6594_BUCK_3,
			  REGULATOR_VOLTAGE, tps6594_bucks_ops, TPS65224_MASK_BUCKS_VSET,
			  TPS6594_REG_BUCKX_VOUT_1(2),
			  TPS65224_MASK_BUCKS_VSET,
			  TPS6594_REG_BUCKX_CTRL(2),
			  TPS6594_BIT_BUCK_EN, 0, 0, tps65224_bucks_2_3_4_ranges,
			  4, 0, 0, NULL, 0, 0),
	TPS6594_REGULATOR("BUCK4", "buck4", TPS6594_BUCK_4,
			  REGULATOR_VOLTAGE, tps6594_bucks_ops, TPS65224_MASK_BUCKS_VSET,
			  TPS6594_REG_BUCKX_VOUT_1(3),
			  TPS65224_MASK_BUCKS_VSET,
			  TPS6594_REG_BUCKX_CTRL(3),
			  TPS6594_BIT_BUCK_EN, 0, 0, tps65224_bucks_2_3_4_ranges,
			  4, 0, 0, NULL, 0, 0),
};

static const struct tps6594_regulator_irq_type tps6594_buck1_irq_types[] = {
