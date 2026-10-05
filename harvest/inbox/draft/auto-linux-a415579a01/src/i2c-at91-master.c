// AUTO-DRAFT from torvalds/linux PR #7704c4c5bb127673b4f0ead839919db573559e38
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
static void at91_calc_twi_clock(struct at91_twi_dev *dev)
{
	int ckdiv, cdiv, div, hold = 0, filter_width = 0;
	struct at91_twi_pdata *pdata = dev->pdata;
	int offset = pdata->clk_offset;
	int max_ckdiv = pdata->clk_max_div;
	struct i2c_timings timings, *t = &timings;

	i2c_parse_fw_timings(dev->dev, t, true);

	div = max(0, (int)DIV_ROUND_UP(clk_get_rate(dev->clk),
				       2 * t->bus_freq_hz) - offset);
	ckdiv = fls(div >> 8);
	cdiv = div >> ckdiv;

	if (ckdiv > max_ckdiv) {
		dev_warn(dev->dev, "%d exceeds ckdiv max value which is %d.\n",
			 ckdiv, max_ckdiv);
		ckdiv = max_ckdiv;
		cdiv = 255;
	}

	if (pdata->has_hold_field) {
		/*
		 * hold time = HOLD + 3 x T_peripheral_clock
		 * Use clk rate in kHz to prevent overflows when computing
		 * hold.
		 */
		hold = DIV_ROUND_UP(t->sda_hold_ns
				    * (clk_get_rate(dev->clk) / 1000), 1000000);
		hold -= 3;
		if (hold < 0)
			hold = 0;
		if (hold > AT91_TWI_CWGR_HOLD_MAX) {
			dev_warn(dev->dev,
				 "HOLD field set to its maximum value (%d instead of %d)\n",
				 AT91_TWI_CWGR_HOLD_MAX, hold);
			hold = AT91_TWI_CWGR_HOLD_MAX;
		}
	}

	if (pdata->has_adv_dig_filtr) {
		/*
		 * filter width = 0 to AT91_TWI_FILTR_THRES_MAX
		 * peripheral clocks
		 */
		filter_width = DIV_ROUND_UP(t->digital_filter_width_ns
				* (clk_get_rate(dev->clk) / 1000), 1000000);
		if (filter_width > AT91_TWI_FILTR_THRES_MAX) {
			dev_warn(dev->dev,
				"Filter threshold set to its maximum value (%d instead of %d)\n",
				AT91_TWI_FILTR_THRES_MAX, filter_width);
			filter_width = AT91_TWI_FILTR_THRES_MAX;
		}
	}

	dev->twi_cwgr_reg = (ckdiv << 16) | (cdiv << 8) | cdiv
			    | AT91_TWI_CWGR_HOLD(hold);

	dev->filter_width = filter_width;

	dev_dbg(dev->dev, "cdiv %d ckdiv %d hold %d (%d ns), filter_width %d (%d ns)\n",
		cdiv, ckdiv, hold, t->sda_hold_ns, filter_width,
		t->digital_filter_width_ns);
}
/* …（同文件无关代码省略）… */
static int at91_init_twi_recovery_gpio(struct platform_device *pdev,
				       struct at91_twi_dev *dev)
{
	struct i2c_bus_recovery_info *rinfo = &dev->rinfo;

	rinfo->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (!rinfo->pinctrl) {
		dev_info(dev->dev, "pinctrl unavailable, bus recovery not supported\n");
		return 0;
	}
	if (IS_ERR(rinfo->pinctrl)) {
		dev_info(dev->dev, "can't get pinctrl, bus recovery not supported\n");
		return PTR_ERR(rinfo->pinctrl);
	}
	dev->adapter.bus_recovery_info = rinfo;

	return 0;
}
/* …（同文件无关代码省略）… */
static int at91_twi_recover_bus_cmd(struct i2c_adapter *adap)
{
	struct at91_twi_dev *dev = i2c_get_adapdata(adap);

	dev->transfer_status |= at91_twi_read(dev, AT91_TWI_SR);
	if (!(dev->transfer_status & AT91_TWI_SDA)) {
		dev_dbg(dev->dev, "SDA is down; sending bus clear command\n");
		if (dev->use_alt_cmd) {
			unsigned int acr;

			acr = at91_twi_read(dev, AT91_TWI_ACR);
			acr &= ~AT91_TWI_ACR_DATAL_MASK;
			at91_twi_write(dev, AT91_TWI_ACR, acr);
		}
		at91_twi_write(dev, AT91_TWI_CR, AT91_TWI_CLEAR);
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int at91_init_twi_recovery_info(struct platform_device *pdev,
				       struct at91_twi_dev *dev)
{
	struct i2c_bus_recovery_info *rinfo = &dev->rinfo;
	bool has_clear_cmd = dev->pdata->has_clear_cmd;

	if (!has_clear_cmd)
		return at91_init_twi_recovery_gpio(pdev, dev);

	rinfo->recover_bus = at91_twi_recover_bus_cmd;
	dev->adapter.bus_recovery_info = rinfo;

	return 0;
}
/* …（同文件无关代码省略）… */
	at91_calc_twi_clock(dev);

	rc = at91_init_twi_recovery_info(pdev, dev);
	if (rc == -EPROBE_DEFER)
		return rc;

	dev->adapter.algo = &at91_twi_algorithm;
	dev->adapter.quirks = &at91_twi_quirks;
