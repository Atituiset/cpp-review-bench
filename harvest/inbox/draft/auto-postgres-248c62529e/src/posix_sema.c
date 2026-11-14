// AUTO-DRAFT from postgres/postgres PR #70c4a57b64d39c526eb4c44a925eb92a6e56f3c5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
 * posix_sema.c
 *	  Implement PGSemaphores using POSIX semaphore facilities
 *
 * We prefer the unnamed style of POSIX semaphore (the kind made with
 * sem_init).  We can cope with the kind made with sem_open, however.
 *
 * In either implementation, typedef PGSemaphore is equivalent to "sem_t *".
 * With unnamed semaphores, the sem_t structs live in an array in shared
 * memory.  With named semaphores, that's not true because we cannot persuade
 * sem_open to do its allocation there.  Therefore, the named-semaphore code
 * *does not cope with EXEC_BACKEND*.  The sem_t structs will just be in the
 * postmaster's private memory, where they are successfully inherited by
 * forked backends, but they could not be accessed by exec'd backends.
 *
 *
 * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
/* …（同文件无关代码省略）… */
 */
#include "postgres.h"

#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#include "miscadmin.h"
#include "storage/ipc.h"
#include "storage/pg_sema.h"
#include "storage/shmem.h"


/* see file header comment */
#if defined(USE_NAMED_POSIX_SEMAPHORES) && defined(EXEC_BACKEND)
#error cannot use named POSIX semaphores with EXEC_BACKEND
#endif

typedef union SemTPadded
{
	sem_t		pgsem;
/* …（同文件无关代码省略）… */
typedef struct PGSemaphoreData
{
	SemTPadded	sem_padded;
} PGSemaphoreData;

#define PG_SEM_REF(x)	(&(x)->sem_padded.pgsem)

#define IPCProtection	(0600)	/* access/modify by user only */

#ifdef USE_NAMED_POSIX_SEMAPHORES
static sem_t **mySemPointers;	/* keep track of created semaphores */
#else
static PGSemaphore sharedSemas; /* array of PGSemaphoreData in shared memory */
#endif
static int	numSems;			/* number of semas acquired so far */
static int	maxSems;			/* allocated size of above arrays */

#ifdef USE_NAMED_POSIX_SEMAPHORES
static int	nextSemKey;			/* next name to try */
#endif


static void ReleaseSemaphores(int status, Datum arg);


#ifdef USE_NAMED_POSIX_SEMAPHORES

/*
 * PosixSemaphoreCreate
 *
 * Attempt to create a new named semaphore.
 *
 * If we fail with a failure code other than collision-with-existing-sema,
 * print out an error and abort.  Other types of errors suggest nonrecoverable
 * problems.
 */
static sem_t *
PosixSemaphoreCreate(void)
{
	int			semKey;
	char		semname[64];
	sem_t	   *mySem;

	for (;;)
	{
		semKey = nextSemKey++;

		snprintf(semname, sizeof(semname), "/pgsql-%d", semKey);

		mySem = sem_open(semname, O_CREAT | O_EXCL,
						 (mode_t) IPCProtection, (unsigned) 1);

#ifdef SEM_FAILED
		if (mySem != (sem_t *) SEM_FAILED)
			break;
#else
		if (mySem != (sem_t *) (-1))
			break;
#endif

		/* Loop if error indicates a collision */
		if (errno == EEXIST || errno == EACCES || errno == EINTR)
			continue;

		/*
		 * Else complain and abort
		 */
		elog(FATAL, "sem_open(\"%s\") failed: %m", semname);
	}

	/*
	 * Unlink the semaphore immediately, so it can't be accessed externally.
	 * This also ensures that it will go away if we crash.
	 */
	sem_unlink(semname);

	return mySem;
}
#else							/* !USE_NAMED_POSIX_SEMAPHORES */

/*
 * PosixSemaphoreCreate
 *
/* …（同文件无关代码省略）… */
	if (sem_init(sem, 1, 1) < 0)
		elog(FATAL, "sem_init failed: %m");
}
#endif							/* USE_NAMED_POSIX_SEMAPHORES */


/*
/* …（同文件无关代码省略）… */
static void
PosixSemaphoreKill(sem_t *sem)
{
#ifdef USE_NAMED_POSIX_SEMAPHORES
	/* Got to use sem_close for named semaphores */
	if (sem_close(sem) < 0)
		elog(LOG, "sem_close failed: %m");
#else
	/* Got to use sem_destroy for unnamed semaphores */
	if (sem_destroy(sem) < 0)
		elog(LOG, "sem_destroy failed: %m");
#endif
}


/* …（同文件无关代码省略）… */
void
PGSemaphoreShmemRequest(int maxSemas)
{
#ifdef USE_NAMED_POSIX_SEMAPHORES
	/* No shared memory needed in this case */
#else
	/* Need a PGSemaphoreData per semaphore */
	ShmemRequestStruct(.name = "Semaphores",
					   .size = mul_size(maxSemas, sizeof(PGSemaphoreData)),
					   .ptr = (void **) &sharedSemas,
		);
#endif
}

/*
/* …（同文件无关代码省略）… */
 * callback to release them.
 *
 * In the Posix implementation, we acquire semaphores on-demand; the
 * maxSemas parameter is just used to size the arrays.  For unnamed
 * semaphores, there is an array of PGSemaphoreData structs in shared memory.
 * For named semaphores, we keep a postmaster-local array of sem_t pointers,
 * which we use for releasing the semaphores when done.
 * (This design minimizes the dependency of postmaster shutdown on the
 * contents of shared memory, which a failed backend might have clobbered.
 * We can't do much about the possibility of sem_destroy() crashing, but
 * we don't have to expose the counters to other processes.)
 */
void
PGSemaphoreInit(int maxSemas)
{
#ifdef USE_NAMED_POSIX_SEMAPHORES
	struct stat statbuf;

	/*
	 * We use the data directory's inode number to seed the search for free
	 * semaphore keys.  This minimizes the odds of collision with other
	 * postmasters, while maximizing the odds that we will detect and clean up
	 * semaphores left over from a crashed postmaster in our own directory.
	 */
	if (stat(DataDir, &statbuf) < 0)
		ereport(FATAL,
				(errcode_for_file_access(),
				 errmsg("could not stat data directory \"%s\": %m",
						DataDir)));
	nextSemKey = statbuf.st_ino;
#endif

#ifdef USE_NAMED_POSIX_SEMAPHORES
	mySemPointers = (sem_t **) malloc(maxSemas * sizeof(sem_t *));
	if (mySemPointers == NULL)
		elog(PANIC, "out of memory");
#endif

	numSems = 0;
	maxSems = maxSemas;

/* …（同文件无关代码省略）… */
static void
ReleaseSemaphores(int status, Datum arg)
{
	int			i;

#ifdef USE_NAMED_POSIX_SEMAPHORES
	for (i = 0; i < numSems; i++)
		PosixSemaphoreKill(mySemPointers[i]);
	free(mySemPointers);
#endif

#ifdef USE_UNNAMED_POSIX_SEMAPHORES
	for (i = 0; i < numSems; i++)
		PosixSemaphoreKill(PG_SEM_REF(sharedSemas + i));
#endif
}

/*
/* …（同文件无关代码省略）… */
	if (numSems >= maxSems)
		elog(PANIC, "too many semap
