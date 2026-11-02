// AUTO-DRAFT from torvalds/linux PR #7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9
#define WATCHDOG_INTERVAL		(HZ >> 1)
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
	/* Could have been rearmed by a stop/start cycle */
	if (!timer_pending(&watchdog_timer)) {
		watchdog_timer.expires += WATCHDOG_INTERVAL;
		add_timer_local(&watchdog_timer);
	}
}
