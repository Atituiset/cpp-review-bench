// AUTO-DRAFT from postgres/postgres PR #d9b5a63f49d9d372a6609243eb0e9417a0a35e6f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
 *		atomic test-and-set only when it appears free.
 *
 *	TAS() and TAS_SPIN() are NOT part of the API, and should never be called
 *	directly.
 *
 *	CAUTION: on some platforms TAS() and/or TAS_SPIN() may sometimes report
 *	failure to acquire a lock even when the lock is not locked.  For example,
 *	on Alpha TAS() will "fail" if interrupted.  Therefore a retry loop must
 *	always be used, even if you are certain the lock is free.
 *
 *	It is the responsibility of these macros to make sure that the compiler
 *	does not re-order accesses to shared memory to precede the actual lock
 *	acquisition, or follow the lock release.  Prior to PostgreSQL 9.5, this
 *	was the caller's responsibility, which meant that callers had to use
 *	volatile-qualified pointers to refer to both the spinlock itself and the
 *	shared data being accessed within the spinlocked critical section.  This
 *	was notationally awkward, easy to forget (and thus error-prone), and
 *	prevented some useful compiler optimizations.  For these reasons, we
 *	now require that the macros themselves prevent compiler re-ordering,
 *	so that the caller doesn't need to take special precautions.
 *
 *	On platforms with weak memory ordering, the TAS(), TAS_SPIN(), and
 *	S_UNLOCK() macros must further include hardware-level memory fence
/* …（同文件无关代码省略）… */
 *
 *	On most supported platforms, TAS() uses a tas() function written
 *	in assembly language to execute a hardware atomic-test-and-set
 *	instruction.  Equivalent OS-supplied mutex routines could be used too.
 *
 *
 * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
/* …（同文件无关代码省略）… */
typedef unsigned char slock_t;
/* …（同文件无关代码省略）… */
#define TAS(lock) tas(lock)
/* …（同文件无关代码省略）… */
static inline int
tas(volatile slock_t *lock)
{
	slock_t		_res = 1;

	/*
	 * Use a non-locking test before asserting the bus lock.  Note that the
	 * extra test appears to be a small loss on some x86 platforms and a small
	 * win on others; it's by no means clear that we should keep it.
	 *
	 * When this was last tested, we didn't have separate TAS() and TAS_SPIN()
	 * macros.  Nowadays it probably would be better to do a non-locking test
	 * in TAS_SPIN() but not in TAS(), like on x86_64, but no-one's done the
	 * testing to verify that.  Without some empirical evidence, better to
	 * leave it alone.
	 */
	__asm__ __volatile__(
		"	cmpb	$0,%1		\n"
		"	jne		TAS%=_out	\n"
		"	lock				\n"
		"	xchgb	%0,%1		\n"
		"TAS%=_out: \n"
:		"+q"(_res), "+m"(*lock)
:		/* no inputs */
:		"memory", "cc");
	return (int) _res;
}
/* …（同文件无关代码省略）… */
#define S_UNLOCK(lock) __sync_lock_release(lock)
/* …（同文件无关代码省略）… */
#endif	/* !defined(TAS) */


/* Blow up if we didn't have any way to do spinlocks */
#ifndef TAS
#error PostgreSQL does not have spinlock support on this platform.  Please report this to pgsql-bugs@lists.postgresql.org.
#endif


/*
 * Default Definitions - override these above as needed.
 */

#if !defined(S_LOCK)
#define S_LOCK(lock) \
	(TAS(lock) ? s_lock((lock), __FILE__, __LINE__, __func__) : 0)
#endif	 /* S_LOCK */

#if !defined(S_UNLOCK)
/* …（同文件无关代码省略）… */
#define SPIN_DELAY()	((void) 0)
#endif	 /* SPIN_DELAY */

#if !defined(TAS_SPIN)
#define TAS_SPIN(lock)	TAS(lock)
#endif	 /* TAS_SPIN */


/*
 * Platform-independent out-of-line support routines
 */
extern int s_lock(volatile slock_t *lock, const char *file, int line, const char *func);

/* Support for dynamic adjustment of spins_per_delay */
#define DEFAULT_SPINS_PER_DELAY  100
