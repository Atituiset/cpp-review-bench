// AUTO-DRAFT from torvalds/linux PR #c339f4010dea8b0d754f84ace70cd1b089878ef8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
#define ASPIRE_EC_EVENT			0x05
/* …（同文件无关代码省略）… */
#define ASPIRE_EC_EVENT_WATCHDOG	0x20
#define ASPIRE_EC_EVENT_KBD_BKL_ON	0x57
#define ASPIRE_EC_EVENT_KBD_BKL_OFF	0x58
#define ASPIRE_EC_EVENT_LID_CLOSE	0x9b
#define ASPIRE_EC_EVENT_LID_OPEN	0x9c
#define ASPIRE_EC_EVENT_BKL_UNBLANKED	0x9d
#define ASPIRE_EC_EVENT_BKL_BLANKED	0x9e
#define ASPIRE_EC_EVENT_FG_INF_CHG	0x85
#define ASPIRE_EC_EVENT_FG_STA_CHG	0xc6
#define ASPIRE_EC_EVENT_HPD_DIS		0xa3
#define ASPIRE_EC_EVENT_HPD_CON		0xa4
/* …（同文件无关代码省略）… */
#define ASPIRE_EC_RAM_READ		0x20
#define ASPIRE_EC_RAM_WRITE		0x21
/* …（同文件无关代码省略）… */
#define ASPIRE_EC_RAM_WATCHDOG		0x19
#define ASPIRE_EC_WATCHDOG_BIT		BIT(6)
/* …（同文件无关代码省略）… */
struct aspire_ec {
	struct i2c_client *client;
	struct power_supply *bat_psy;
	struct power_supply *adp_psy;
	struct input_dev *idev;

	bool bridge_configured;
	struct drm_bridge bridge;
	struct work_struct work;
};
/* …（同文件无关代码省略）… */
static int aspire_ec_ram_read(struct i2c_client *client, u8 off, u8 *data, u8 data_len)
{
	i2c_smbus_write_byte_data(client, ASPIRE_EC_RAM_READ, off);
	i2c_smbus_read_i2c_block_data(client, ASPIRE_EC_RAM_READ, data_len, data);
	return 0;
}
/* …（同文件无关代码省略）… */
static int aspire_ec_ram_write(struct i2c_client *client, u8 off, u8 data)
{
	u8 tmp[2] = {off, data};

	i2c_smbus_write_i2c_block_data(client, ASPIRE_EC_RAM_WRITE, sizeof(tmp), tmp);
	return 0;
}
/* …（同文件无关代码省略）… */
static irqreturn_t aspire_ec_irq_handler(int irq, void *data)
{
	struct aspire_ec *ec = data;
	int id;
	u8 tmp;

	/*
	 * The original ACPI firmware actually has a small sleep in the handler.
	 *
	 * It seems like in most cases it's not needed but when the device
	 * just exits suspend, our i2c driver has a brief time where data
	 * transfer is not possible yet. So this delay allows us to suppress
	 * quite a bunch of spurious error messages in dmesg. Thus it's kept.
	 */
	usleep_range(15000, 30000);

	id = i2c_smbus_read_byte_data(ec->client, ASPIRE_EC_EVENT);
	if (id < 0) {
		dev_err(&ec->client->dev, "Failed to read event id: %pe\n", ERR_PTR(id));
		return IRQ_HANDLED;
	}

	switch (id) {
	case 0x0: /* No event */
		break;

	case ASPIRE_EC_EVENT_WATCHDOG:
		/*
		 * Here acpi responds to the event and clears some bit.
		 * Notify (\_SB.I2C3.BAT1, 0x81) // Information Change
		 * Notify (\_SB.I2C3.ADP1, 0x80) // Status Change
		 */
		aspire_ec_ram_read(ec->client, ASPIRE_EC_RAM_WATCHDOG, &tmp, sizeof(tmp));
		tmp &= ~ASPIRE_EC_WATCHDOG_BIT;
		aspire_ec_ram_write(ec->client, ASPIRE_EC_RAM_WATCHDOG, tmp);
		break;

	case ASPIRE_EC_EVENT_LID_CLOSE:
		/* Notify (\_SB.LID0, 0x80) // Status Change */
		input_report_switch(ec->idev, SW_LID, 1);
		input_sync(ec->idev);
		break;

	case ASPIRE_EC_EVENT_LID_OPEN:
		/* Notify (\_SB.LID0, 0x80) // Status Change */
		input_report_switch(ec->idev, SW_LID, 0);
		input_sync(ec->idev);
		break;

	case ASPIRE_EC_EVENT_FG_INF_CHG:
		/* Notify (\_SB.I2C3.BAT1, 0x81) // Information Change */
		fallthrough;
	case ASPIRE_EC_EVENT_FG_STA_CHG:
		/* Notify (\_SB.I2C3.BAT1, 0x80) // Status Change */
		power_supply_changed(ec->bat_psy);
		power_supply_changed(ec->adp_psy);
		break;

	case ASPIRE_EC_EVENT_HPD_DIS:
		if (ec->bridge_configured)
			drm_bridge_hpd_notify(&ec->bridge, connector_status_disconnected);
		break;

	case ASPIRE_EC_EVENT_HPD_CON:
		if (ec->bridge_configured)
			drm_bridge_hpd_notify(&ec->bridge, connector_status_connected);
		break;

	case ASPIRE_EC_EVENT_BKL_BLANKED:
	case ASPIRE_EC_EVENT_BKL_UNBLANKED:
		/* Display backlight blanked on FN+F6. No action needed. */
		break;

	case ASPIRE_EC_EVENT_KBD_BKL_ON:
	case ASPIRE_EC_EVENT_KBD_BKL_OFF:
		/*
		 * There is a keyboard backlight connector on Aspire 1 that is
		 * controlled by FN+F8. There is no kb backlight on the device though.
		 * Seems like this is used on other devices like Acer Spin 7.
		 * No action needed.
		 */
		break;

	default:
		dev_warn(&ec->client->dev, "Unknown event id=0x%x\n", id);
	}

	return IRQ_HANDLED;
}
/* …（同文件无关代码省略）… */
					aspire_ec_irq_handler, IRQF_ONESHOT,
					dev_name(dev), ec);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to request irq\n");

	return 0;
}
