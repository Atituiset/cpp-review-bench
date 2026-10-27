// AUTO-DRAFT from torvalds/linux PR #5b93dc1c0c61fd0fe575cb473e9e25f929fda509
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
 * Author: Richard Purdie <rpurdie@openedhand.com>
 */

#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/timer.h>
/* …（同文件无关代码省略）… */
static inline bool
trigger_relevant(struct led_classdev *led_cdev, struct led_trigger *trig)
{
	return !trig->trigger_type || trig->trigger_type == led_cdev->trigger_type;
}

ssize_t led_trigger_write(struct file *filp, struct kobject *kobj,
			  const struct bin_attribute *bin_attr, char *buf,
			  loff_t pos, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
/* …（同文件无关代码省略）… */
	mutex_unlock(&led_cdev->led_access);
	return ret;
}
EXPORT_SYMBOL_GPL(led_trigger_write);

__printf(3, 4)
static int led_trigger_snprintf(char *buf, ssize_t size, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	if (size <= 0)
		i = vsnprintf(NULL, 0, fmt, args);
	else
		i = vscnprintf(buf, size, fmt, args);
	va_end(args);

	return i;
}
/* …（同文件无关代码省略）… */
static int led_trigger_format(char *buf, size_t size,
			      struct led_classdev *led_cdev)
{
	struct led_trigger *trig;
	int len = led_trigger_snprintf(buf, size, "%s",
				       led_cdev->trigger ? "none" : "[none]");

	if (led_cdev->default_trigger)
		len += led_trigger_snprintf(buf + len, size - len, " default");

	list_for_each_entry(trig, &trigger_list, next_trig) {
		bool hit;

		if (!trigger_relevant(led_cdev, trig))
			continue;

		hit = led_cdev->trigger && !strcmp(led_cdev->trigger->name, trig->name);

		len += led_trigger_snprintf(buf + len, size - len,
					    " %s%s%s", hit ? "[" : "",
					    trig->name, hit ? "]" : "");
	}

	len += led_trigger_snprintf(buf + len, size - len, "\n");

	return len;
}
/* …（同文件无关代码省略）… */
 * attribute, which is not limited by length. This is _not_ good design, do not
 * copy it.
 */
ssize_t led_trigger_read(struct file *filp, struct kobject *kobj,
			const struct bin_attribute *attr, char *buf,
			loff_t pos, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	void *data;
	int len;

	down_read(&triggers_list_lock);
	down_read(&led_cdev->trigger_lock);

	len = led_trigger_format(NULL, 0, led_cdev);
	data = kvmalloc(len + 1, GFP_KERNEL);
	if (!data) {
		up_read(&led_cdev->trigger_lock);
		up_read(&triggers_list_lock);
		return -ENOMEM;
	}
	len = led_trigger_format(data, len + 1, led_cdev);

	up_read(&led_cdev->trigger_lock);
	up_read(&triggers_list_lock);

	len = memory_read_from_buffer(buf, count, &pos, data, len);

	kvfree(data);

	return len;
}
EXPORT_SYMBOL_GPL(led_trigger_read);

/* Caller must ensure led_cdev->trigger_lock held */
int led_trigger_set(struct led_classdev *led_cdev, struct led_trigger *trig)
{
	char *event = NULL;
	char *envp[2];
/* …（同文件无关代码省略）… */
		led_cdev->trigger_data = NULL;
		led_cdev->activated = false;
		led_cdev->flags &= ~LED_INIT_DEFAULT_TRIGGER;
		led_set_brightness(led_cdev, LED_OFF);
	}
	if (trig) {
		spin_lock(&trig->leddev_list_lock);
/* …（同文件无关代码省略）… */

	return ret;
}
EXPORT_SYMBOL_GPL(led_trigger_set);

void led_trigger_remove(struct led_classdev *led_cdev)
{
	down_write(&led_cdev->trigger_lock);
	led_trigger_set(led_cdev, NULL);
	up_write(&led_cdev->trigger_lock);
}
EXPORT_SYMBOL_GPL(led_trigger_remove);

static bool led_match_default_trigger(struct led_classdev *led_cdev,
				      struct led_trigger *trig)
{
/* …（同文件无关代码省略）… */
int led_trigger_register(struct led_trigger *trig)
{
	struct led_classdev *led_cdev;
	struct led_trigger *_trig;

	spin_lock_init(&trig->leddev_list_lock);
	INIT_LIST_HEAD(&trig->led_cdevs);

	down_write(&triggers_list_lock);
	/* Make sure the trigger's name isn't already in use */
	list_for_each_entry(_trig, &trigger_list, next_trig) {
		if (!strcmp(_trig->name, trig->name) &&
		    (trig->trigger_type == _trig->trigger_type ||
		     !trig->trigger_type || !_trig->trigger_type)) {
			up_write(&triggers_list_lock);
			return -EEXIST;
		}
	}
	/* Add to the list of led triggers */
	list_add_tail(&trig->next_trig, &trigger_list);
	up_write(&triggers_list_lock);

	/* Register with any LEDs that have this as a default trigger */
	down_read(&leds_list_lock);
	list_for_each_entry(led_cdev, &leds_list, node) {
		down_write(&led_cdev->trigger_lock);
		if (!led_cdev->trigger && led_cdev->default_trigger)
			led_match_default_trigger(led_cdev, trig);
		up_write(&led_cdev->trigger_lock);
	}
	up_read(&leds_list_lock);

	return 0;
}
/* …（同文件无关代码省略）… */
void led_trigger_unregister(struct led_trigger *trig)
{
	struct led_classdev *led_cdev;

	if (list_empty_careful(&trig->next_trig))
		return;

	/* Remove from the list of led triggers */
	down_write(&triggers_list_lock);
	list_del_init(&trig->next_trig);
	up_write(&triggers_list_lock);

	/* Remove anyone actively using this trigger */
	down_read(&leds_list_lock);
	list_for_each_entry(led_cdev, &leds_list, node) {
		down_write(&led_cdev->trigger_lock);
		if (led_cdev->trigger == trig)
			led_trigger_set(led_cdev, NULL);
		up_write(&led_cdev->trigger_lock);
	}
	up_read(&leds_list_lock);
}
/* …（同文件无关代码省略）… */
static void devm_led_trigger_release(struct device *dev, void *res)
{
	led_trigger_unregister(*(struct led_trigger **)res);
}
/* …（同文件无关代码省略）… */
int devm_led_trigger_register(struct device *dev,
			      struct led_trigger *trig)
{
	struct led_trigger **dr;
	int rc;

	dr = devres_alloc(devm_led_trigger_release, sizeof(*dr),
			  GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	*dr = trig;

	rc = led_trigger_register(trig);
	if (rc)
		devres_free(dr);
	else
		devres_add(dev, dr);

	return rc;
}
EXPORT_SYMBOL_GPL(devm_led_trigger_register);

/* Simple LED Trigger Interface */

void led_trigger_event(struct led_trigger *trig,
			enum led_brightness brightness)
{
	struct led_classdev *led_cdev;

	if (!trig)
		return;

	trig->brightness
