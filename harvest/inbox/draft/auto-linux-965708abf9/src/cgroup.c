// AUTO-DRAFT from torvalds/linux PR #c3b510de420d70def08190083d388e0873c1aa84
	spin_lock_irq(&css_set_lock);
	cset = task_css_set(current);
	get_css_set(cset);
	if (kargs->cgrp)  // <<< BUG ANCHOR
		kargs->kill_seq = kargs->cgrp->kill_seq;
	else
		kargs->kill_seq = cset->dfl_cgrp->kill_seq;
	spin_unlock_irq(&css_set_lock);

	if (!(kargs->flags & CLONE_INTO_CGROUP)) {
/* …（同文件无关代码省略）… */

	put_css_set(cset);
	kargs->cgrp = dst_cgrp;
	return ret;

err:
