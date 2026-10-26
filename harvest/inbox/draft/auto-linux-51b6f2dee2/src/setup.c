// AUTO-DRAFT from torvalds/linux PR #44dc57ed1e77a23248ef28732486f4b7e9468cf7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <linux/platform_data/tmio.h>
#include <linux/platform_data/tsc2007.h>
#include <linux/platform_device.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/machine.h>
#include <linux/sh_eth.h>
/* …（同文件无关代码省略）… */
/* TouchScreen */
#define IRQ0 evt2irq(0x600)

static int ts_get_pendown_state(struct device *dev)
{
	int val = 0;
	gpio_free(GPIO_FN_INTC_IRQ0);
	gpio_request(GPIO_PTZ0, NULL);
	gpio_direction_input(GPIO_PTZ0);

	val = gpio_get_value(GPIO_PTZ0);

	gpio_free(GPIO_PTZ0);
	gpio_request(GPIO_FN_INTC_IRQ0, NULL);

	return val ? 0 : 1;
}

static int ts_init(void)
{
	gpio_request(GPIO_FN_INTC_IRQ0, NULL);
	return 0;
}

static struct tsc2007_platform_data tsc2007_info = {
	.model			= 2007,
	.x_plate_ohms		= 180,
	.get_pendown_state	= ts_get_pendown_state,
	.init_platform_hw	= ts_init,
};

static struct i2c_board_info ts_i2c_clients = {
	I2C_BOARD_INFO("tsc2007", 0x48),
	.type		= "tsc2007",
	.platform_data	= &tsc2007_info,
	.irq		= IRQ0,
};

static struct regulator_consumer_supply cn12_power_consumers[] =
/* …（同文件无关代码省略）… */
		gpio_direction_output(GPIO_PTF4, 1);

		/* enable TouchScreen */
		i2c_register_board_info(0, &ts_i2c_clients, 1);
		irq_set_irq_type(IRQ0, IRQ_TYPE_LEVEL_LOW);
	}

	/* enable CEU0 */
