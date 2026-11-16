// AUTO-DRAFT from torvalds/linux PR #4b15991b3be43e9581aca489ec30a351310bb49d
#define CGROUP_FILE_NOTIFY_MIN_INTV	DIV_ROUND_UP(HZ, 100)
/* …（同文件无关代码省略）… */
	if (!READ_ONCE(cfile->kn))
		return;
  // <<< BUG ANCHOR
	last = READ_ONCE(cfile->notified_at);
	next = last + CGROUP_FILE_NOTIFY_MIN_INTV;
	if (time_in_range(jiffies, last, next)) {
		timer_reduce(&cfile->notify_timer, next);
		if (timer_pending(&cfile->notify_timer))
			return;
	}

	spin_lock_irqsave(&cfile->lock, flags);
	if (cfile->kn) {
		kn = cfile->kn;
		kernfs_get(kn);
		WRITE_ONCE(cfile->notified_at, jiffies);
	}
	spin_unlock_irqrestore(&cfile->lock, flags);
