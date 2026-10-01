// AUTO-DRAFT from postgres/postgres PR #c712b0d49b971fe7cf521c057bc7adb63b17760c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct ShmemIndexEnt ShmemIndexEnt;
/* …（同文件无关代码省略）… */
typedef struct
{
	ShmemStructOpts *options;
	ShmemRequestKind kind;

	/* InitShmemIndexEntry() sets this pointer when the area is allocated */
	ShmemIndexEnt *index_entry;
} ShmemRequest;
/* …（同文件无关代码省略）… */
	MemoryContext oldcontext;
	ShmemRequest *request;

	/* Check the options */
	if (options->name == NULL)
		elog(ERROR, "shared memory request is missing 'name' option");

	if (IsUnderPostmaster)
	{
		if (options->size <= 0 && options->size != SHMEM_ATTACH_UNKNOWN_SIZE)
			elog(ERROR, "invalid size %zd for shared memory request for \"%s\"",
				 options->size, options->name);
	}
	else
	{
		if (options->size == SHMEM_ATTACH_UNKNOWN_SIZE)
			elog(ERROR, "SHMEM_ATTACH_UNKNOWN_SIZE cannot be used during startup");
		if (options->size <= 0)
			elog(ERROR, "invalid size %zd for shared memory request for \"%s\"",
				 options->size, options->name);
	}

	if (options->alignment != 0 && pg_nextpower2_size_t(options->alignment) != options->alignment)
		elog(ERROR, "invalid alignment %zu for shared memory request for \"%s\"",
			 options->alignment, options->name);

	/* Check that we're in the right state */
	if (shmem_request_state != SRS_REQUESTING)
		elog(ERROR, "ShmemRequestStruct can only be called from a shmem_request callback");

	/* Check that it's not already registered in this process */
	foreach_ptr(ShmemRequest, existing, pending_shmem_requests)
	{
/* …（同文件无关代码省略）… */
ResetShmemAllocator(void)
{
	Assert(!IsUnderPostmaster);
	shmem_request_state = SRS_INITIAL;

	pending_shmem_requests = NIL;
