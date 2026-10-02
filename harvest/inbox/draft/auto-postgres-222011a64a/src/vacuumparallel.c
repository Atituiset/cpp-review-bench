// AUTO-DRAFT from postgres/postgres PR #42e96cf2fe095c822f76bc94669ea875cb1e3351
#include "executor/instrument.h"
#include "optimizer/paths.h"
#include "pgstat.h"
#include "storage/bufmgr.h"
#include "storage/proc.h"
#include "tcop/tcopprot.h"
/* …（同文件无关代码省略）… */
	pg_atomic_fetch_add_u32(&pv_shared_cost_params->generation, 1);
}

/*
 * Compute the number of parallel worker processes to request.  Both index
 * vacuum and index cleanup can be executed with parallel workers.
