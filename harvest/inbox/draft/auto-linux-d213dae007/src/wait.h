// AUTO-DRAFT from torvalds/linux PR #cea5f91daa3ac86c98b63b3a2e89da622670ccf6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define WQ_FLAG_EXCLUSIVE	0x01
/* …（同文件无关代码省略）… */
struct wait_queue_entry {
	unsigned int		flags;
	void			*private;
	wait_queue_func_t	func;
	struct list_head	entry;
};
/* …（同文件无关代码省略）… */
#define ___wait_cond_timeout(condition)						\
({										\
	bool __cond = (condition);						\
	if (__cond && !__ret)							\
		__ret = 1;							\
	__cond || !__ret;							\
})
/* …（同文件无关代码省略）… */
 * otherwise.
 */

#define ___wait_event(wq_head, condition, state, exclusive, ret, cmd)		\
({										\
	__label__ __out;							\
	struct wait_queue_entry __wq_entry;					\
	long __ret = ret;	/* explicit shadow */				\
										\
	init_wait_entry(&__wq_entry, exclusive ? WQ_FLAG_EXCLUSIVE : 0);	\
	for (;;) {								\
		long __int = prepare_to_wait_event(&wq_head, &__wq_entry, state);\
										\
/* …（同文件无关代码省略）… */
})

#define __wait_event(wq_head, condition)					\
	(void)___wait_event(wq_head, condition, TASK_UNINTERRUPTIBLE, 0, 0,	\
			    schedule())

/**
 * wait_event - sleep until a condition gets true
/* …（同文件无关代码省略）… */
} while (0)

#define __wait_event_freezable(wq_head, condition)				\
	___wait_event(wq_head, condition, (TASK_INTERRUPTIBLE|TASK_FREEZABLE),	\
			0, 0, schedule())

/**
 * wait_event_freezable - sleep (or freeze) until a condition gets true
/* …（同文件无关代码省略）… */
})

#define __wait_event_exclusive_cmd(wq_head, condition, cmd1, cmd2)		\
	(void)___wait_event(wq_head, condition, TASK_UNINTERRUPTIBLE, 1, 0,	\
			    cmd1; schedule(); cmd2)
/*
 * Just like wait_event_cmd(), except it sets exclusive flag
 */
/* …（同文件无关代码省略）… */
} while (0)

#define __wait_event_interruptible(wq_head, condition)				\
	___wait_event(wq_head, condition, TASK_INTERRUPTIBLE, 0, 0,		\
		      schedule())

/**
 * wait_event_interruptible - sleep until a condition gets true
/* …（同文件无关代码省略）… */
})

#define __wait_event_interruptible_exclusive(wq, condition)			\
	___wait_event(wq, condition, TASK_INTERRUPTIBLE, 1, 0,			\
		      schedule())

#define wait_event_interruptible_exclusive(wq, condition)			\
({										\
/* …（同文件无关代码省略）… */
})

#define __wait_event_killable_exclusive(wq, condition)				\
	___wait_event(wq, condition, TASK_KILLABLE, 1, 0,			\
		      schedule())

#define wait_event_killable_exclusive(wq, condition)				\
({										\
/* …（同文件无关代码省略）… */


#define __wait_event_freezable_exclusive(wq, condition)				\
	___wait_event(wq, condition, (TASK_INTERRUPTIBLE|TASK_FREEZABLE), 1, 0,\
			schedule())

#define wait_event_freezable_exclusive(wq, condition)				\
({										\
/* …（同文件无关代码省略）… */
do {										\
	might_sleep();								\
	if (!(condition))							\
		___wait_event(wq_head, condition, TASK_IDLE, 0, 0, schedule());	\
} while (0)

/**
/* …（同文件无关代码省略）… */
do {										\
	might_sleep();								\
	if (!(condition))							\
		___wait_event(wq_head, condition, TASK_IDLE, 1, 0, schedule());	\
} while (0)

#define __wait_event_idle_timeout(wq_head, condition, timeout)			\
/* …（同文件无关代码省略）… */

#define __wait_event_idle_exclusive_timeout(wq_head, condition, timeout)	\
	___wait_event(wq_head, ___wait_cond_timeout(condition),			\
		      TASK_IDLE, 1, timeout,					\
		      __ret = schedule_timeout(__ret))

/**
/* …（同文件无关代码省略）… */


#define __wait_event_killable(wq, condition)					\
	___wait_event(wq, condition, TASK_KILLABLE, 0, 0, schedule())

/**
 * wait_event_killable - sleep until a condition gets true
/* …（同文件无关代码省略）… */
})

#define __wait_event_state_exclusive(wq, condition, state)			\
	___wait_event(wq, condition, state, 1, 0, schedule())

#define wait_event_state_exclusive(wq, condition, state)			\
({										\
