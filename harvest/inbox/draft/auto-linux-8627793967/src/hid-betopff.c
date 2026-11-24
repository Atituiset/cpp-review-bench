// AUTO-DRAFT from torvalds/linux PR #8d7fe9885c62fc9a95bf55d9db244d9672a8aadc
 *
 *  0x20bc:0x5500 "BTP2185 V2 BFM mode Joystick"
 *   - tested with BTP2171s.
 *  Copyright (c) 2014 Huang Bo <huangbobupt@163.com>
 */

/* …（同文件无关代码省略）… */
static const struct hid_device_id betop_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_BETOP_2185BFM, 0x2208) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_BETOP_2185PC, 0x5506) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_BETOP_2185V2PC, 0x1850) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_BETOP_2185V2BFM, 0x5500) },
	{ }
