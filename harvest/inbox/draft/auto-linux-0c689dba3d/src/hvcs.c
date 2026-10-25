// AUTO-DRAFT from torvalds/linux PR #cb8fef4b07f71da01843457b66e579ad248a625e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
goto register_fail;
	}

	hvcs_pi_buff = (unsigned long *) __get_free_page(GFP_KERNEL);
	if (!hvcs_pi_buff) {
		rc = -ENOMEM;
		goto buff_alloc_fail;
	return 0;

kthread_fail:
	free_page((unsigned long)hvcs_pi_buff);
buff_alloc_fail:
	tty_unregister_driver(hvcs_tty_driver);
register_fail:
	kthread_stop(hvcs_task);

	spin_lock(&hvcs_pi_lock);
	free_page((unsigned long)hvcs_pi_buff);
	hvcs_pi_buff = NULL;
	spin_unlock(&hvcs_pi_lock);
