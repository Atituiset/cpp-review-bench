// AUTO-DRAFT from postgres/postgres PR #c712b0d49b971fe7cf521c057bc7adb63b17760c
#include "fmgr.h"
#include "miscadmin.h"
#include "storage/shmem.h"
#include "utils/guc.h"
#include "utils/injection_point.h"

/* …（同文件无关代码省略）… */
		elog(ERROR, "shmem area not yet initialized");
	PG_RETURN_INT32(TestShmem->attach_count);
}
