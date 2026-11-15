// AUTO-DRAFT from torvalds/linux PR #54975c6523dc77862c25deeb5fa395990b8d3fc4
#define REG_CR2		0x01
/* …（同文件无关代码省略）… */
#define CR2_ALERTS	BIT(6)
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
#define REG_TACH(ch)		(0x52 + (ch) * 2)
#define REG_TEMP_INPUT(ch)	(0x56 + (ch) * 2)
#define REG_TEMP_MAX(ch)	(0x06 + (ch) * 2)
#define REG_TEMP_CRIT(ch)	(0x0A + (ch) * 2)

#define TEMP11_FROM_REG(reg)	((reg) / 32 * 125)
#define TEMP11_TO_REG(val)	(DIV_ROUND_CLOSEST(clamp_val((val), -128000, \
/* …（同文件无关代码省略）… */

static int tach_to_rpm(u16 tach)
{
	if (tach == 0)
		tach = 1;

	return 60 * 100000 / tach / 2;
}
/* …（同文件无关代码省略）… */

	dev_set_drvdata(dev, state);

	/* Set alert output to comparator mode */
	ret = regmap_set_bits(state->regmap, REG_CR2, CR2_ALERTS);
	if (ret)
		return dev_err_probe(dev, ret, "cannot write register\n");
