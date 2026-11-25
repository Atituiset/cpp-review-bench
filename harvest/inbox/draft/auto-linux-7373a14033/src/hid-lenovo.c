// AUTO-DRAFT from torvalds/linux PR #8d7fe9885c62fc9a95bf55d9db244d9672a8aadc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

#define LENOVO_KEY_MICMUTE KEY_F20
/* …（同文件无关代码省略）… */
#define TP_X12_RAW_HOTKEY_FN_F4		0x00020003
#define TP_X12_RAW_HOTKEY_FN_F8		0x38001003
#define TP_X12_RAW_HOTKEY_FN_F10	0x00000803
#define TP_X12_RAW_HOTKEY_FN_F12	0x00000403
#define TP_X12_RAW_HOTKEY_FN_SPACE	0x18001003
/* …（同文件无关代码省略）… */
static inline void report_key_event(struct input_dev *input, int keycode)
{
	input_report_key(input, keycode, 1);
	input_report_key(input, keycode, 0);
	input_sync(input);
}
/* …（同文件无关代码省略）… */
static int lenovo_raw_event_TP_X12_tab(struct hid_device *hdev, u32 raw_data)
{
	struct hid_input *hidinput;
	struct input_dev *input = NULL;

	/* Iterate through all associated input devices */
	list_for_each_entry(hidinput, &hdev->inputs, list) {
		input = hidinput->input;
		if (!input)
			continue;

		switch (raw_data) {
			/* fn-F20 being used here for MIC mute*/
		case TP_X12_RAW_HOTKEY_FN_F4:
			report_key_event(input, LENOVO_KEY_MICMUTE);
			return 1;
		/* Power-mode or Airplane mode will be called based on the device*/
		case TP_X12_RAW_HOTKEY_FN_F8:
			/*
			 * TP X12 TAB uses Fn-F8 calls Airplanemode
			 * Whereas TP X12 TAB2 uses Fn-F8 for toggling
			 * Power modes
			 */
			if (hdev->product == USB_DEVICE_ID_LENOVO_X12_TAB) {
				report_key_event(input, KEY_RFKILL);
				return 1;
			}
			report_key_event(input, KEY_PERFORMANCE);
			return 1;
		case TP_X12_RAW_HOTKEY_FN_F10:
			/* TAB1 has PICKUP Phone and TAB2 use Snipping tool*/
			(hdev->product == USB_DEVICE_ID_LENOVO_X12_TAB) ?
			report_key_event(input, KEY_PICKUP_PHONE) :
			report_key_event(input, KEY_SELECTIVE_SCREENSHOT);
			return 1;
		case TP_X12_RAW_HOTKEY_FN_F12:
			/* BookMarks/STAR key*/
			report_key_event(input, KEY_BOOKMARKS);
			return 1;
		case TP_X12_RAW_HOTKEY_FN_SPACE:
			/* Keyboard LED backlight toggle*/
			report_key_event(input, KEY_KBDILLUMTOGGLE);
			return 1;
		default:
			break;
		}
	}
	return 0;
}
/* …（同文件无关代码省略）… */
		      size >= 4 && report->id == 0x03))
		return lenovo_raw_event_TP_X12_tab(hdev, get_unaligned_le32(data));

	return 0;
}

/* …（同文件无关代码省略）… */
{
	int ret;

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "hid_parse failed\n");
/* …（同文件无关代码省略）… */
	{ HID_USB_DEVICE(USB_VENDOR_ID_IBM, USB_DEVICE_ID_IBM_SCROLLPOINT_800DPI_OPTICAL_PRO) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_LENOVO, USB_DEVICE_ID_LENOVO_SCROLLPOINT_OPTICAL) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_LENOVO, USB_DEVICE_ID_LENOVO_TP10UBKBD) },
	/*
	 * Note bind to the HID_GROUP_GENERIC group, so that we only bind to the keyboard
	 * part, while letting hid-multitouch.c handle the touchpad and trackpoint.
