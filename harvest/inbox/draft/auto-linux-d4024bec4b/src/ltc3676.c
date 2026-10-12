// AUTO-DRAFT from torvalds/linux PR #5906f156e8818d29839d7d7280b691f627b59883
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdint.h>
  // <<< BUG ANCHOR
#define LTC3676_BUCK1     0x01
/* …（同文件无关代码省略）… */
#define LTC3676_IRQSTAT   0x15
/* …（同文件无关代码省略）… */
#define LTC3676_CLIRQ     0x1F
/* …（同文件无关代码省略）… */
static inline unsigned int ltc3676_scale(unsigned int uV, u32 r1, u32 r2)
{
	uint64_t tmp;
	if (uV == 0)
		return 0;
	tmp = (uint64_t)uV * r1;
	do_div(tmp, r2);
	return uV + (unsigned int)tmp;
}
/* …（同文件无关代码省略）… */
static int ltc3676_of_parse_cb(struct device_node *np,
			       const struct regulator_desc *desc,
			       struct regulator_config *config)
{
	struct ltc3676 *ltc3676 = config->driver_data;
	struct regulator_desc *rdesc = &ltc3676->regulator_descs[desc->id];
	u32 r[2];
	int ret;

	/* LDO3 has a fixed output */
	if (desc->id == LTC3676_LDO3)
		return 0;

	ret = of_property_read_u32_array(np, "lltc,fb-voltage-divider", r, 2);
	if (ret) {
		dev_err(ltc3676->dev, "Failed to parse voltage divider: %d\n",
			ret);
		return ret;
	}

	rdesc->min_uV = ltc3676_scale(desc->min_uV, r[0], r[1]);
	rdesc->uV_step = ltc3676_scale(desc->uV_step, r[0], r[1]);
	rdesc->fixed_uV = ltc3676_scale(desc->fixed_uV, r[0], r[1]);

	return 0;
}
/* …（同文件无关代码省略）… */
#define LTC3676_REG(_id, _name, _ops, en_reg, en_bit, dvba_reg, dvb_mask)   \
	[LTC3676_ ## _id] = {                                        \
		.name = #_name,                                \
		.of_match = of_match_ptr(#_name),              \
		.regulators_node = of_match_ptr("regulators"), \
		.of_parse_cb = ltc3676_of_parse_cb,            \
		.n_voltages = (dvb_mask) + 1,                  \
		.min_uV = (dvba_reg) ? 412500 : 0,             \
		.uV_step = (dvba_reg) ? 12500 : 0,             \
		.ramp_delay = (dvba_reg) ? 800 : 0,            \
		.fixed_uV = (dvb_mask) ? 0 : 725000,           \
		.ops = &ltc3676_ ## _ops ## _regulator_ops,    \
		.type = REGULATOR_VOLTAGE,                     \
		.id = LTC3676_ ## _id,                         \
		.owner = THIS_MODULE,                          \
		.vsel_reg = (dvba_reg),                        \
		.vsel_mask = (dvb_mask),                       \
		.enable_reg = (en_reg),                        \
		.enable_mask = (1 << en_bit),                  \
	}
/* …（同文件无关代码省略）… */
#define LTC3676_FIXED_REG(_id, _name, _en_reg, _en_bit)                \
	LTC3676_REG(_id, _name, fixed, LTC3676_ ## _en_reg, _en_bit, 0, 0)
/* …（同文件无关代码省略）… */
	LTC3676_FIXED_REG(LDO4, ldo4, LDOB, 2),
};

static bool ltc3676_readable_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case LTC3676_BUCK1 ... LTC3676_IRQSTAT:
/* …（同文件无关代码省略）… */
	return false;
}

static bool ltc3676_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
/* …（同文件无关代码省略）… */
static const struct regmap_config ltc3676_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.writeable_reg = ltc3676_readable_writeable_reg,
	.readable_reg = ltc3676_readable_writeable_reg,
	.volatile_reg = ltc3676_volatile_reg,
	.max_register = LTC3676_CLIRQ,
	.use_single_read = true,
