// AUTO-DRAFT from torvalds/linux PR #7be4bc52fd9e0e55dc9a2e0232b89427d2c4b614
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include "base.h"
#include "power/power.h"

/* …（同文件无关代码省略）… */

	dev = bus_find_device_by_name(bus, NULL, buf);
	if (dev && dev->driver == drv) {
		device_driver_detach(dev);
		err = count;
	}
/* …（同文件无关代码省略）… */

	dev = bus_find_device_by_name(bus, NULL, buf);
	if (dev && driver_match_device(drv, dev)) {
		err = device_driver_attach(drv, dev);
		if (!err) {
			/* success */
