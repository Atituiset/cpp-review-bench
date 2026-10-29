// AUTO-DRAFT from torvalds/linux PR #1cb6cfab0916d18f88d44e4a23806b6c4cb355f8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
	mutex_unlock(&bus->bus_lock);
}

static int sdw_slave_clk_stop_callback(struct sdw_slave *slave,
				       enum sdw_clk_stop_mode mode,
				       enum sdw_clk_stop_type type)
{
	int ret = 0;
/* …（同文件无关代码省略）… */
		struct sdw_driver *drv = drv_to_sdw_driver(dev->driver);

		if (drv->ops && drv->ops->clk_stop)
			ret = drv->ops->clk_stop(slave, mode, type);
	}

	mutex_unlock(&slave->sdw_dev_lock);
/* …（同文件无关代码省略）… */
}

static int sdw_slave_clk_stop_prepare(struct sdw_slave *slave,
				      enum sdw_clk_stop_mode mode,
				      bool prepare)
{
	bool wake_en;
/* …（同文件无关代码省略）… */
	if (prepare) {
		val = SDW_SCP_SYSTEMCTRL_CLK_STP_PREP;

		if (mode == SDW_CLK_STOP_MODE1)
			val |= SDW_SCP_SYSTEMCTRL_CLK_STP_MODE1;

		if (wake_en)
/* …（同文件无关代码省略）… */
		/* Identify if Slave(s) are available on Bus */
		is_slave = true;

		ret = sdw_slave_clk_stop_callback(slave,
						  SDW_CLK_STOP_MODE0,
						  SDW_CLK_PRE_PREPARE);
		if (ret < 0 && ret != -ENODATA) {
			dev_err(&slave->dev, "clock stop pre-prepare cb failed:%d\n", ret);
			return ret;
/* …（同文件无关代码省略）… */
		if (!slave->prop.simple_clk_stop_capable) {
			simple_clk_stop = false;

			ret = sdw_slave_clk_stop_prepare(slave,
							 SDW_CLK_STOP_MODE0,
							 true);
			if (ret < 0 && ret != -ENODATA) {
				dev_err(&slave->dev, "clock stop prepare failed:%d\n", ret);
				return ret;
/* …（同文件无关代码省略）… */
		    slave->status != SDW_SLAVE_ALERT)
			continue;

		ret = sdw_slave_clk_stop_callback(slave,
						  SDW_CLK_STOP_MODE0,
						  SDW_CLK_POST_PREPARE);

		if (ret < 0 && ret != -ENODATA) {
			dev_err(&slave->dev, "clock stop post-prepare cb failed:%d\n", ret);
/* …（同文件无关代码省略）… */
		/* Identify if Slave(s) are available on Bus */
		is_slave = true;

		ret = sdw_slave_clk_stop_callback(slave, SDW_CLK_STOP_MODE0,
						  SDW_CLK_PRE_DEPREPARE);
		if (ret < 0)
			dev_warn(&slave->dev, "clock stop pre-deprepare cb failed:%d\n", ret);

		/* Only de-prepare a Slave device if needed */
		if (!slave->prop.simple_clk_stop_capable) {
			simple_clk_stop = false;

			ret = sdw_slave_clk_stop_prepare(slave, SDW_CLK_STOP_MODE0,
							 false);

			if (ret < 0)
				dev_warn(&slave->dev, "clock stop deprepare failed:%d\n", ret);
		}
/* …（同文件无关代码省略）… */
		    slave->status != SDW_SLAVE_ALERT)
			continue;

		ret = sdw_slave_clk_stop_callback(slave, SDW_CLK_STOP_MODE0,
						  SDW_CLK_POST_DEPREPARE);
		if (ret < 0)
			dev_warn(&slave->dev, "clock stop post-deprepare cb failed:%d\n", ret);
	}
