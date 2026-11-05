// AUTO-DRAFT from postgres/postgres PR #953c6dabdb52663a6b96ad354b33496284d8ed73
#include "miscadmin.h"
#include "storage/aio_internal.h"
#include "storage/fd.h"
#include "storage/proc.h"
#include "storage/shmem.h"
#include "storage/lwlock.h"
/* …（同文件无关代码省略）… */
		);
}

static void
pgaio_uring_shmem_init(void *arg)
{
/* …（同文件无关代码省略）… */

		LWLockInitialize(&context->completion_lock, LWTRANCHE_AIO_URING_COMPLETION);
	}
}

static void
