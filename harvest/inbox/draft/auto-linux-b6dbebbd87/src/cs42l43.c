// AUTO-DRAFT from torvalds/linux PR #d857e1f3faf998f2614dcdb52a90a27bc83bd71d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define CS42L43_SDW_ATTACH_TIMEOUT_MS		5000
/* …（同文件无关代码省略）… */
static int cs42l43_wait_for_attach(struct cs42l43 *cs42l43)
{
	int ret;

	ret = sdw_slave_wait_for_init(cs42l43->sdw, CS42L43_SDW_ATTACH_TIMEOUT_MS);
	if (ret)
		return ret;

	regcache_cache_only(cs42l43->regmap, false);

	/* The hardware requires enabling OSC_DIV before doing any SoundWire reads. */
	if (cs42l43->sdw)
		regmap_write(cs42l43->regmap, CS42L43_OSC_DIV_SEL,
			     CS42L43_OSC_DIV2_EN_MASK);

	return 0;
}
/* …（同文件无关代码省略）… */
{
	struct cs42l43 *cs42l43 = container_of(work, struct cs42l43, boot_work);
	unsigned int devid, revid, otp;
	int ret;

	ret = cs42l43_wait_for_attach(cs42l43);
/* …（同文件无关代码省略）… */

	switch (devid) {
	case CS42L43_DEVID_VAL:
	case CS42L43B_DEVID_VAL:
		if (devid != cs42l43->variant_id) {
			dev_err(cs42l43->dev,
				"Device ID (0x%06x) does not match variant ID (0x%06lx)\n",
				devid, cs42l43->variant_id);
			goto err;
		}
		break;
	default:
		dev_err(cs42l43->dev, "Unrecognised devid: 0x%06x\n", devid);
		goto err;
	}

	ret = regmap_read(cs42l43->regmap, CS42L43_REVID, &revid);
	if (ret) {
		dev_err(cs42l43->dev, "Failed to read rev: %d\n", ret);
/* …（同文件无关代码省略）… */
static int cs42l43_power_down(struct cs42l43 *cs42l43)
{
	int ret;

	ret = regulator_disable(cs42l43->vdd_d);
	if (ret) {
		dev_err(cs42l43->dev, "Failed to disable vdd-d: %d\n", ret);
		return ret;
	}

	ret = regulator_bulk_disable(CS42L43_N_SUPPLIES, cs42l43->core_supplies);
	if (ret) {
		dev_err(cs42l43->dev, "Failed to disable core supplies: %d\n", ret);
		return ret;
	}

	gpiod_set_raw_value_cansleep(cs42l43->reset, 0);

	ret = regulator_disable(cs42l43->vdd_p);
	if (ret) {
		dev_err(cs42l43->dev, "Failed to disable vdd-p: %d\n", ret);
		return ret;
	}

	return 0;
}

static void cs42l43_dev_remove(void *data)
{
	struct cs42l43 *cs42l43 = data;

	cancel_work_sync(&cs42l43->boot_work);

	cs42l43_power_down(cs42l43);
}

int cs42l43_dev_probe(struct cs42l43 *cs42l43)
{
	int i, ret;
/* …（同文件无关代码省略）… */
	if (ret)
		return ret;

	ret = devm_add_action_or_reset(cs42l43->dev, cs42l43_dev_remove, cs42l43);
	if (ret)
		return ret;
