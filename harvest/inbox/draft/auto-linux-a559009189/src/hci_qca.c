// AUTO-DRAFT from torvalds/linux PR #1e575bda30acc0bf01f799cee798d4a1c100d538
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
  // <<< BUG ANCHOR
#define CMD_TRANS_TIMEOUT		msecs_to_jiffies(100)
/* …（同文件无关代码省略）… */
#define MAX_INIT_RETRIES 3
/* …（同文件无关代码省略）… */
enum qca_memdump_states {
	QCA_MEMDUMP_IDLE,
	QCA_MEMDUMP_COLLECTING,
	QCA_MEMDUMP_COLLECTED,
	QCA_MEMDUMP_TIMEOUT,
};
/* …（同文件无关代码省略）… */
struct qca_memdump_info {
	u32 current_seq_no;
	u32 received_dump;
	u32 ram_dump_size;
};
/* …（同文件无关代码省略）… */
struct qca_data {
	struct hci_uart *hu;
	struct sk_buff *rx_skb;
	struct sk_buff_head txq;
	struct sk_buff_head tx_wait_q;	/* HCI_IBS wait queue	*/
	struct sk_buff_head rx_memdump_q;	/* Memdump wait queue	*/
	spinlock_t hci_ibs_lock;	/* HCI_IBS state lock	*/
	u8 tx_ibs_state;	/* HCI_IBS transmit side power state*/
	u8 rx_ibs_state;	/* HCI_IBS receive side power state */
	bool tx_vote;		/* Clock must be on for TX */
	bool rx_vote;		/* Clock must be on for RX */
	struct timer_list tx_idle_timer;
	u32 tx_idle_delay;
	struct timer_list wake_retrans_timer;
	u32 wake_retrans;
	struct workqueue_struct *workqueue;
	struct work_struct ws_awake_rx;
	struct work_struct ws_awake_device;
	struct work_struct ws_rx_vote_off;
	struct work_struct ws_tx_vote_off;
	struct work_struct ctrl_memdump_evt;
	struct delayed_work ctrl_memdump_timeout;
	struct qca_memdump_info *qca_memdump;
	unsigned long flags;
	struct dentry *debugfs_ibs_dir;
	struct completion drop_ev_comp;
	wait_queue_head_t suspend_wait_q;
	enum qca_memdump_states memdump_state;
	struct mutex hci_memdump_lock;

	u16 fw_version;
	u16 controller_id;
	/* For debugging purpose */
	u64 ibs_sent_wacks;
	u64 ibs_sent_slps;
	u64 ibs_sent_wakes;
	u64 ibs_recv_wacks;
	u64 ibs_recv_slps;
	u64 ibs_recv_wakes;
	u64 vote_last_jif;
	u32 vote_on_ms;
	u32 vote_off_ms;
	u64 tx_votes_on;
	u64 rx_votes_on;
	u64 tx_votes_off;
	u64 rx_votes_off;
	u64 votes_on;
	u64 votes_off;
};
/* …（同文件无关代码省略）… */
enum qca_speed_type {
	QCA_INIT_SPEED = 1,
	QCA_OPER_SPEED
};
/* …（同文件无关代码省略）… */
struct qca_power {
	struct device *dev;
	struct regulator_bulk_data *vreg_bulk;
	int num_vregs;
	bool vregs_on;
	struct pwrseq_desc *pwrseq;
};
/* …（同文件无关代码省略）… */
struct qca_serdev {
	struct hci_uart	 serdev_hu;
	struct gpio_desc *bt_en;
	struct gpio_desc *sw_ctrl;
	struct clk	 *susclk;
	enum qca_btsoc_type btsoc_type;
	struct qca_power *bt_power;
	u32 init_speed;
	u32 oper_speed;
	bool bdaddr_property_broken;
	bool support_hfp_hw_offload;
	const char *firmware_name[2];
};
/* …（同文件无关代码省略）… */
static enum qca_btsoc_type qca_soc_type(struct hci_uart *hu)
{
	enum qca_btsoc_type soc_type;

	if (hu->serdev) {
		struct qca_serdev *qsd = serdev_device_get_drvdata(hu->serdev);

		soc_type = qsd->btsoc_type;
	} else {
		soc_type = QCA_ROME;
	}

	return soc_type;
}
/* …（同文件无关代码省略）… */
		if (!test_bit(QCA_MEMDUMP_COLLECTION, &qca->flags)) {
			bt_dev_err(hu->hdev, "QCA: Discarding other packets");
			kfree(qca_memdump);
			kfree_skb(skb);
			mutex_unlock(&qca->hci_memdump_lock);
			return;
/* …（同文件无关代码省略）… */
static uint8_t qca_get_baudrate_value(int speed)
{
	switch (speed) {
	case 9600:
		return QCA_BAUDRATE_9600;
	case 19200:
		return QCA_BAUDRATE_19200;
	case 38400:
		return QCA_BAUDRATE_38400;
	case 57600:
		return QCA_BAUDRATE_57600;
	case 115200:
		return QCA_BAUDRATE_115200;
	case 230400:
		return QCA_BAUDRATE_230400;
	case 460800:
		return QCA_BAUDRATE_460800;
	case 500000:
		return QCA_BAUDRATE_500000;
	case 921600:
		return QCA_BAUDRATE_921600;
	case 1000000:
		return QCA_BAUDRATE_1000000;
	case 2000000:
		return QCA_BAUDRATE_2000000;
	case 3000000:
		return QCA_BAUDRATE_3000000;
	case 3200000:
		return QCA_BAUDRATE_3200000;
	case 3500000:
		return QCA_BAUDRATE_3500000;
	default:
		return QCA_BAUDRATE_115200;
	}
}
/* …（同文件无关代码省略）… */
static int qca_set_baudrate(struct hci_dev *hdev, uint8_t baudrate)
{
	struct hci_uart *hu = hci_get_drvdata(hdev);
	struct qca_data *qca = hu->priv;
	struct sk_buff *skb;
	u8 cmd[] = { 0x01, 0x48, 0xFC, 0x01, 0x00 };

	if (baudrate > QCA_BAUDRATE_3200000)
		return -EINVAL;

	cmd[4] = baudrate;

	skb = bt_skb_alloc(sizeof(cmd), GFP_KERNEL);
	if (!skb) {
		bt_dev_err(hdev, "Failed to allocate baudrate packet");
		return -ENOMEM;
	}

	/* Assign commands to change baudrate and packet type. */
	skb_put_data(skb, cmd, sizeof(cmd));
	hci_skb_pkt_type(skb) = HCI_COMMAND_PKT;

	skb_queue_tail(&qca->txq, skb);
	hci_uart_tx_wakeup(hu);

	/* Wait for the baudrate change request to be sent */

	while (!skb_queue_empty(&qca->txq))
		usleep_range(100, 200);

	if (hu->serdev)
		serdev_device_wait_until_sent(hu->serdev,
		      CMD_TRANS_TIMEOUT);

	/* Give the controller time to process the request */
	switch (qca_soc_type(hu)) {
	case QCA_WCN3950:
	case QCA_WCN3988:
	case QCA_WCN3990:
	case QCA_WCN3991:
	case QCA_WCN3998:
	case QCA_WCN6750:
	case QCA_WCN6855:
	case QCA_WCN7850:
		usleep_range(1000, 10000);
		break;

	default:
		msleep(300);
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static inline void host_set_baudrate(struct hci_uart *hu, unsigned int speed)
{
	if (hu->serdev)
		serdev_device_set_baudrate(hu->serdev, speed);
	else
		hci_uart_set_baudrate(hu, speed);
}
/* …（同文件无关代码省略）… */
static int qca_send_power_pulse(struct hci_uart *hu, bool on)
{
	int timeout = CMD_TRANS_TIMEOUT;
	int ret;
	u8 cmd = on ? QCA_WCN3990_POWERON_PULSE : QCA_WCN3990_POWEROFF_PULSE;

	/* These power pulses are single byte command which are sent
	 * at required baudrate to wcn3990. On wcn3990, we have an external
	 * circuit at Tx pin which decodes the pulse sent at specific baudrate.
	 * For example, wcn3990 supports RF COEX antenna for both Wi-Fi/BT
	 * and also we use the same power inputs to turn on and off for
	 * Wi-Fi/BT. Powering up the power sources will not enable BT, until
	 * we send a power on pulse at 115200 bps. This algorithm will help to
	 * save power. Disabling hardware flow control is mandatory while
	 * sending power pulses to SoC.
	 */
	bt_dev_dbg(hu->hdev, "sending power pulse %02x to controller", cmd);

	serdev_device_write_flush(hu->serdev);
	hci_uart_set_flow_control(hu, true);
	ret = serdev_device_write_buf(hu->serdev, &cmd, sizeof(cmd));
	if (ret < 0) {
		bt_dev_err(hu->hdev, "failed to send power pulse %02x", cmd);
		return ret;
	}

	serdev_device_wait_until_sent(hu->serdev, timeout);
	hci_uart_set_flow_control(hu, false);

	/* Give to controller time to boot/shutdown */
	if (on)
		msleep(100);
	else
		usleep_range(1000, 10000);

	return 0;
}
/* …（同文件无关代码省略）… */
static unsigned int qca_get_speed(struct hci_uart *hu,
				  enum qca_speed_type speed_type)
{
	unsigned int speed = 0;

	if (speed_type == QCA_INIT_SPEED) {
		if (hu->init_speed)
			speed = hu->init_speed;
		else if (hu->proto->init_speed)
			speed = hu->proto->init_speed;
	} else {
		if (hu->oper_speed)
			speed = hu->oper_speed;
		else if (hu->proto->oper_speed)
			speed = hu->proto->oper_speed;
	}

	return speed;
}
/* …（同文件无关代码省略）… */
	case QCA_WCN3991:
	case QCA_WCN3998:
	case QCA_WCN6750:
	case QCA_WCN6855:
	case QCA_WCN7850:
		if (!qca_get_speed(hu, QCA_INIT_SPEED) &&
/* …（同文件无关代码省略）… */
static int qca_set_speed(struct hci_uart *hu, enum qca_speed_type speed_type)
{
	unsigned int speed, qca_baudrate;
	struct qca_data *qca = hu->priv;
	int ret = 0;

	if (speed_type == QCA_INIT_SPEED) {
		speed = qca_get_speed(hu, QCA_INIT_SPEED);
		if (speed)
			host_set_baudrate(hu, speed);
	} else {
		enum qca_btsoc_type soc_type = qca_soc_type(hu);

		speed = qca_get_speed(hu, QCA_OPER_SPEED);
		if (!speed)
			return 0;

		/* Disable flow control for wcn3990 to deassert RTS while
		 * changing the baudrate of chip and host.
		 */
		switch (soc_type) {
		case QCA_WCN3950:
		case QCA_WCN3988:
		case QCA_WCN3990:
		case QCA_WCN3991:
		case QCA_WCN3998:
		case QCA_WCN6750:
		case QCA_WCN6855:
		case QCA_WCN7850:
			hci_uart_set_flow_control(hu, true);
			break;

		default:
			break;
		}

		switch (soc_type) {
		case QCA_WCN3990:
			reinit_completion(&qca->drop_ev_comp);
			set_bit(QCA_DROP_VENDOR_EVENT, &qca->flags);
			break;

		default:
			break;
		}

		qca_baudrate = qca_get_baudrate_value(speed);
		bt_dev_dbg(hu->hdev, "Set UART speed to %d", speed);
		ret = qca_set_baudrate(hu->hdev, qca_baudrate);
		if (ret)
			goto error;

		host_set_baudrate(hu, speed);

error:
		switch (soc_type) {
		case QCA_WCN3950:
		case QCA_WCN3988:
		case QCA_WCN3990:
		case QCA_WCN3991:
		case QCA_WCN3998:
		case QCA_WCN6750:
		case QCA_WCN6855:
		case QCA_WCN7850:
			hci_uart_set_flow_control(hu, false);
			break;

		default:
			break;
		}

		switch (soc_type) {
		case QCA_WCN3990:
			/* Wait for the controller to send the vendor event
			 * for the baudrate change command.
			 */
			if (!wait_for_completion_timeout(&qca->drop_ev_comp,
						 msecs_to_jiffies(100))) {
				bt_dev_err(hu->hdev,
					   "Failed to change controller baudrate\n");
				ret = -ETIMEDOUT;
			}

			clear_bit(QCA_DROP_VENDOR_EVENT, &qca->flags);
			break;

		default:
			break;
		}
	}

	return ret;
}
/* …（同文件无关代码省略）… */
	return wakeup;
}

static int qca_port_reopen(struct hci_uart *hu)
{
	int ret;
/* …（同文件无关代码省略）… */
	 * Without this, we will have RTS and CTS synchronization
	 * issues.
	 */
	serdev_device_close(hu->serdev);
	ret = serdev_device_open(hu->serdev);
	if (ret) {
		bt_dev_err(hu->hdev, "failed to open port");
		return ret;
/* …（同文件无关代码省略）… */
static int qca_regulator_init(struct hci_uart *hu)
{
	enum qca_btsoc_type soc_type = qca_soc_type(hu);
	struct qca_serdev *qcadev;
	int ret;
	bool sw_ctrl_state;

	/* Check for vregs status, may be hci down has turned
	 * off the voltage regulator.
	 */
	qcadev = serdev_device_get_drvdata(hu->serdev);

	if (!qcadev->bt_power->vregs_on) {
		serdev_device_close(hu->serdev);
		ret = qca_regulator_enable(qcadev);
		if (ret)
			return ret;

		ret = serdev_device_open(hu->serdev);
		if (ret) {
			bt_dev_err(hu->hdev, "failed to open port");
			return ret;
		}
	}

	switch (soc_type) {
	case QCA_WCN3950:
	case QCA_WCN3988:
	case QCA_WCN3990:
	case QCA_WCN3991:
	case QCA_WCN3998:
		/* Forcefully enable wcn399x to enter in to boot mode. */
		host_set_baudrate(hu, 2400);
		ret = qca_send_power_pulse(hu, false);
		if (ret)
			return ret;
		break;

	default:
		break;
	}

	/* For wcn6750 need to enable gpio bt_en */
	if (qcadev->bt_en) {
		gpiod_set_value_cansleep(qcadev->bt_en, 0);
		msleep(50);
		gpiod_set_value_cansleep(qcadev->bt_en, 1);
		msleep(50);
		if (qcadev->sw_ctrl) {
			sw_ctrl_state = gpiod_get_value_cansleep(qcadev->sw_ctrl);
			bt_dev_dbg(hu->hdev, "SW_CTRL is %d", sw_ctrl_state);
		}
	}

	qca_set_speed(hu, QCA_INIT_SPEED);

	switch (soc_type) {
	case QCA_WCN3950:
	case QCA_WCN3988:
	case QCA_WCN3990:
	case QCA_WCN3991:
	case QCA_WCN3998:
		ret = qca_send_power_pulse(hu, true);
		if (ret)
			return ret;
		break;

	default:
		break;
	}

	return qca_port_reopen(hu);
}
/* …（同文件无关代码省略）… */
	case QCA_WCN3991:
	case QCA_WCN3998:
	case QCA_WCN6750:
	case QCA_WCN6855:
	case QCA_WCN7850:
		ret = qca_regulator_init(hu);
/* …（同文件无关代码省略）… */
	case QCA_WCN3991:
	case QCA_WCN3998:
	case QCA_WCN6750:
	case QCA_WCN6855:
	case QCA_WCN7850:
		if (qcadev && qcadev->bdaddr_property_broken)
/* …（同文件无关代码省略）… */
	case QCA_WCN3991:
	case QCA_WCN3998:
	case QCA_WCN6750:
	case QCA_WCN6855:
	case QCA_WCN7850:
		break;
/* …（同文件无关代码省略）… */
		if (retries < MAX_INIT_RETRIES) {
			bt_dev_warn(hdev, "Retry BT power ON:%d", retries);
			if (hu->serdev) {
				serdev_device_close(hu->serdev);
				ret = serdev_device_open(hu->serdev);
				if (ret) {
					bt_dev_err(hdev, "failed to open port");
					return ret;
/* …（同文件无关代码省略）… */
		break;

	case QCA_WCN6750:
	case QCA_WCN6855:
		gpiod_set_value_cansleep(qcadev->bt_en, 0);
		msleep(100);
/* …（同文件无关代码省略）… */
static int qca_regulator_enable(struct qca_serdev *qcadev)
{
	struct qca_power *power = qcadev->bt_power;
	int ret;

	if (power->pwrseq)
		return pwrseq_enable(power->pwrseq);

	/* Already enabled */
	if (power->vregs_on)
		return 0;

	BT_DBG("enabling %d regulators)", power->num_vregs);

	ret = regulator_bulk_enable(power->num_vregs, power->vreg_bulk);
	if (ret)
		return ret;

	power->vregs_on = true;

	ret = clk_prepare_enable(qcadev->susclk);
	if (ret)
		qca_regulator_disable(qcadev);

	return ret;
}
/* …（同文件无关代码省略）… */
static void qca_regulator_disable(struct qca_serdev *qcadev)
{
	struct qca_power *power;

	if (!qcadev)
		return;

	power = qcadev->bt_power;

	/* Already disabled? */
	if (!power->vregs_on)
		return;

	regulator_bulk_disable(power->num_vregs, power->vreg_bulk);
	power->vregs_on = false;

	clk_disable_unprepare(qcadev->susclk);
}
/* …（同文件无关代码省略）… */
	case QCA_WCN3991:
	case QCA_WCN3998:
	case QCA_WCN6750:
	case QCA_WCN6855:
	case QCA_WCN7850:
		qcadev->bt_power = devm_kzalloc_obj(&serdev->dev,
/* …（同文件无关代码省略）… */
	case QCA_WCN3991:
	case QCA_WCN3998:
	case QCA_WCN6750:
	case QCA_WCN6855:
	case QCA_WCN7850:
		if (!device_property_present(&serdev->dev, "enable-gpios")) {
/* …（同文件无关代码省略）… */

		if (!qcadev->bt_en &&
		    (data->soc_type == QCA_WCN6750 ||
		     data->soc_type == QCA_WCN6855 ||
		     data->soc_type == QCA_WCN7850))
			power_ctrl_enabled = false;
/* …（同文件无关代码省略）… */
					       GPIOD_IN);
		if (IS_ERR(qcadev->sw_ctrl) &&
		    (data->soc_type == QCA_WCN6750 ||
		     data->soc_type == QCA_WCN6855 ||
		     data->soc_type == QCA_WCN7850)) {
			dev_err(&serdev->dev, "failed to acquire SW_CTRL gpio\n");
/* …（同文件无关代码省略）… */
	case QCA_WCN3991:
	case QCA_WCN3998:
	case QCA_WCN6750:
	case QCA_WCN6855:
	case QCA_WCN7850:
		if (power->vregs_on)
