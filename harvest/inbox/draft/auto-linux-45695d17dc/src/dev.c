// AUTO-DRAFT from torvalds/linux PR #551c722f40809618230001baccf219193e22fc5a
	struct rtc_device *rtc = file->private_data;
	const struct rtc_class_ops *ops = rtc->ops;
	struct rtc_time tm;
	struct rtc_wkalrm alarm;  // <<< BUG ANCHOR
	struct rtc_param param;
	void __user *uarg = (void __user *)arg;
