// AUTO-DRAFT from postgres/postgres PR #6e06f62816fc99e7cc2e5ea8210fe413916d74cf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
#define PG_SEM_REF(x)	(&(x)->sem_padded.pgsem)
/* …（同文件无关代码省略）… */
#endif
static int	numSems;			/* number of semas acquired so far */
static int	maxSems;			/* allocated size of above arrays */
static int	nextSemKey;			/* next name to try */


static void ReleaseSemaphores(int status, Datum arg);
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
PGSemaphoreInit(int maxSemas)
{
	struct stat statbuf;

	/*
/* …（同文件无关代码省略）… */
				(errcode_for_file_access(),
				 errmsg("could not stat data directory \"%s\": %m",
						DataDir)));

#ifdef USE_NAMED_POSIX_SEMAPHORES
	mySemPointers = (sem_t **) malloc(maxSemas * sizeof(sem_t *));
/* …（同文件无关代码省略）… */

	numSems = 0;
	maxSems = maxSemas;
	nextSemKey = statbuf.st_ino;

	on_shmem_exit(ReleaseSemaphores, 0);
}
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
