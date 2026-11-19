// AUTO-DRAFT from torvalds/linux PR #27538caf7f72ce93be9c352148dba4f2274cf300
// SPDX-License-Identifier: GPL-2.0-only
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/prandom.h>
#include <linux/debugfs.h>
#include <linux/sched.h>
/* …（同文件无关代码省略）… */
		return 0;
	}
  // <<< BUG ANCHOR
	prandom_init_once(&fault_rnd_state);

	attr->probability = probability;
	attr->interval = interval;
