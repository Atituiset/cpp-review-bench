// AUTO-DRAFT from torvalds/linux PR #7d51f37ed48e11a0a073ec428abe9d61c4449072
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
static dev_t watchdog_devt;
/* Reference to watchdog device behind /dev/watchdog */
static struct watchdog_core_data *old_wd_data;

static struct kthread_worker *watchdog_kworker;

/* …（同文件无关代码省略）… */
static bool watchdog_past_open_deadline(struct watchdog_core_data *data)
{
	return ktime_after(ktime_get(), data->open_deadline);
}
/* …（同文件无关代码省略）… */
static void watchdog_set_open_deadline(struct watchdog_core_data *data)
{
	data->open_deadline = open_timeout ?
		ktime_get() + ktime_set(open_timeout, 0) : KTIME_MAX;
}
/* …（同文件无关代码省略）… */
static inline bool watchdog_need_worker(struct watchdog_device *wdd)
{
	/* All variables in milli-seconds */
	unsigned int hm = wdd->max_hw_heartbeat_ms;
	unsigned int t = wdd->timeout * 1000;

	/*
	 * A worker to generate heartbeat requests is needed if all of the
	 * following conditions are true.
	 * - Userspace activated the watchdog.
	 * - The driver provided a value for the maximum hardware timeout, and
	 *   thus is aware that the framework supports generating heartbeat
	 *   requests.
	 * - Userspace requests a longer timeout than the hardware can handle.
	 *
	 * Alternatively, if userspace has not opened the watchdog
	 * device, we take care of feeding the watchdog if it is
	 * running.
	 */
	return (hm && watchdog_active(wdd) && t > hm) ||
		(t && !watchdog_active(wdd) && watchdog_hw_running(wdd));
}
/* …（同文件无关代码省略）… */
static ktime_t watchdog_next_keepalive(struct watchdog_device *wdd)
{
	struct watchdog_core_data *wd_data = wdd->wd_data;
	unsigned int timeout_ms = wdd->timeout * 1000;
	ktime_t keepalive_interval;
	ktime_t last_heartbeat, latest_heartbeat;
	ktime_t virt_timeout;
	unsigned int hw_heartbeat_ms;

	if (watchdog_active(wdd))
		virt_timeout = ktime_add(wd_data->last_keepalive,
					 ms_to_ktime(timeout_ms));
	else
		virt_timeout = wd_data->open_deadline;

	hw_heartbeat_ms = min_not_zero(timeout_ms, wdd->max_hw_heartbeat_ms);
	keepalive_interval = ms_to_ktime(hw_heartbeat_ms / 2);

	/*
	 * To ensure that the watchdog times out wdd->timeout seconds
	 * after the most recent ping from userspace, the last
	 * worker ping has to come in hw_heartbeat_ms before this timeout.
	 */
	last_heartbeat = ktime_sub(virt_timeout, ms_to_ktime(hw_heartbeat_ms));
	latest_heartbeat = ktime_sub(last_heartbeat, ktime_get());
	if (ktime_before(latest_heartbeat, keepalive_interval))
		return latest_heartbeat;
	return keepalive_interval;
}
/* …（同文件无关代码省略）… */
static inline void watchdog_update_worker(struct watchdog_device *wdd)
{
	struct watchdog_core_data *wd_data = wdd->wd_data;

	if (watchdog_need_worker(wdd)) {
		ktime_t t = watchdog_next_keepalive(wdd);

		if (t > 0)
			hrtimer_start(&wd_data->timer, t,
				      HRTIMER_MODE_REL_HARD);
	} else {
		hrtimer_cancel(&wd_data->timer);
	}
}
/* …（同文件无关代码省略）… */
static int __watchdog_ping(struct watchdog_device *wdd)
{
	struct watchdog_core_data *wd_data = wdd->wd_data;
	ktime_t earliest_keepalive, now;
	int err;

	earliest_keepalive = ktime_add(wd_data->last_hw_keepalive,
				       ms_to_ktime(wdd->min_hw_heartbeat_ms));
	now = ktime_get();

	if (ktime_after(earliest_keepalive, now)) {
		hrtimer_start(&wd_data->timer,
			      ktime_sub(earliest_keepalive, now),
			      HRTIMER_MODE_REL_HARD);
		return 0;
	}

	wd_data->last_hw_keepalive = now;

	if (wdd->ops->ping) {
		err = wdd->ops->ping(wdd);  /* ping the watchdog */
		trace_watchdog_ping(wdd, err);
	} else {
		err = wdd->ops->start(wdd); /* restart watchdog */
		trace_watchdog_start(wdd, err);
	}

	if (err == 0)
		watchdog_hrtimer_pretimeout_start(wdd);

	watchdog_update_worker(wdd);

	return err;
}
/* …（同文件无关代码省略）… */
static bool watchdog_worker_should_ping(struct watchdog_core_data *wd_data)
{
	struct watchdog_device *wdd = wd_data->wdd;

	if (!wdd)
		return false;

	if (watchdog_active(wdd))
		return true;

	return watchdog_hw_running(wdd) && !watchdog_past_open_deadline(wd_data);
}
/* …（同文件无关代码省略）… */

	started_at = ktime_get();
	if (watchdog_hw_running(wdd) && wdd->ops->ping) {
		err = __watchdog_ping(wdd);
		if (err == 0) {
			set_bit(WDOG_ACTIVE, &wdd->status);
			watchdog_hrtimer_pretimeout_start(wdd);
		}
	} else {
		err = wdd->ops->start(wdd);
/* …（同文件无关代码省略）… */
static int watchdog_stop(struct watchdog_device *wdd)
{
	int err = 0;

	if (!watchdog_active(wdd))
		return 0;

	if (test_bit(WDOG_NO_WAY_OUT, &wdd->status)) {
		pr_info("watchdog%d: nowayout prevents watchdog being stopped!\n",
			wdd->id);
		return -EBUSY;
	}

	if (wdd->ops->stop) {
		clear_bit(WDOG_HW_RUNNING, &wdd->status);
		err = wdd->ops->stop(wdd);
		if (err < 0) {
			pr_err("watchdog%d: Failed to stop watchdog: %pe\n",
			       wdd->id, ERR_PTR(err));
		}
		trace_watchdog_stop(wdd, err);
	} else {
		set_bit(WDOG_HW_RUNNING, &wdd->status);
	}

	if (err == 0) {
		clear_bit(WDOG_ACTIVE, &wdd->status);
		watchdog_update_worker(wdd);
		watchdog_hrtimer_pretimeout_stop(wdd);
	}

	return err;
}
/* …（同文件无关代码省略）… */
	int err;

	/* Get the corresponding watchdog device */
	if (imajor(inode) == MISC_MAJOR)
		wd_data = old_wd_data;
	else
		wd_data = container_of(inode->i_cdev, struct watchdog_core_data,
				       cdev);

	/* the watchdog is single open! */
	if (test_and_set_bit(_WDOG_DEV_OPEN, &wd_data->status))
		return -EBUSY;

	wdd = wd_data->wdd;

	/*
	 * If the /dev/watchdog device is open, we don't want the module
/* …（同文件无关代码省略）… */
	 */
	wd_data->open_deadline = KTIME_MAX;

	/* dev/watchdog is a virtual (and thus non-seekable) filesystem */
	return stream_open(inode, file);

out_mod:
	module_put(wd_data->wdd->ops->owner);
out_clear:
	clear_bit(_WDOG_DEV_OPEN, &wd_data->status);
	return err;
}

/* …（同文件无关代码省略）… */
static int watchdog_cdev_register(struct watchdog_device *wdd)
{
	struct watchdog_core_data *wd_data;
	int err;

	wd_data = kzalloc_obj(struct watchdog_core_data);
/* …（同文件无关代码省略）… */
	wdd->wd_data = wd_data;

	if (IS_ERR_OR_NULL(watchdog_kworker)) {
		kfree
