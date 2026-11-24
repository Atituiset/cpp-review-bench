// AUTO-DRAFT from torvalds/linux PR #8d7fe9885c62fc9a95bf55d9db244d9672a8aadc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
#define FEATURE_KBD_REPORT_SIZE 64
/* …（同文件无关代码省略）… */

static int asus_kbd_set_report(struct hid_device *hdev, const u8 *buf, size_t buf_size)
{
	u8 *dmabuf __free(kfree) = kmemdup(buf, buf_size, GFP_KERNEL);
	if (!dmabuf)
		return -ENOMEM;

	/*
	 * The report ID should be set from the incoming buffer due to LED and key
	 * interfaces having different pages
	 */
	return hid_hw_raw_request(hdev, buf[0], dmabuf, buf_size, HID_FEATURE_REPORT,
				  HID_REQ_SET_REPORT);
}

static int asus_kbd_init(struct hid_device *hdev, u8 report_id)
{
	/*
	 * The handshake is first sent as a set_report, then retrieved
	 * from a get_report. They should be equal.
	 */
	const u8 buf[] = { report_id, 0x41, 0x53, 0x55, 0x53, 0x20, 0x54,
		     0x65, 0x63, 0x68, 0x2e, 0x49, 0x6e, 0x63, 0x2e, 0x00 };
	int ret;

	ret = asus_kbd_set_report(hdev, buf, sizeof(buf));
	if (ret < 0) {
		hid_err(hdev, "Asus handshake %02x failed to send: %d\n",
			report_id, ret);
		return ret;
	}

	u8 *readbuf __free(kfree) = kzalloc(FEATURE_KBD_REPORT_SIZE, GFP_KERNEL);
	if (!readbuf)
		return -ENOMEM;

	ret = hid_hw_raw_request(hdev, report_id, readbuf,
				 FEATURE_KBD_REPORT_SIZE, HID_FEATURE_REPORT,
				 HID_REQ_GET_REPORT);
	if (ret < 0) {
		hid_warn(hdev, "Asus handshake %02x failed to receive ack: %d\n",
			 report_id, ret);
	} else if (memcmp(readbuf, buf, sizeof(buf)) != 0) {
		hid_warn(hdev, "Asus handshake %02x returned invalid response: %*ph\n",
			 report_id, FEATURE_KBD_REPORT_SIZE, readbuf);
	}

	/*
	 * Do not return error if handshake is wrong until this is
	 * verified to work for all devices.
	 */
	return 0;
}
/* …（同文件无关代码省略）… */
		case 0xe2: /* Mute */
		case 0xe9: /* Volume up */
		case 0xea: /* Volume down */
			return 0;
		default:
			/* Ignore dummy Consumer usages which make the
