// AUTO-DRAFT from postgres/postgres PR #11591f273e6a26fb55728b410538fa2627b19b28
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "executor/instrument.h"
#include "executor/nodeAppend.h"
#include "executor/nodeForeignscan.h"

/*
 * Asynchronously request a tuple from a designed async-capable node.
/* …（同文件无关代码省略）… */
		case T_AppendState:
			ExecAsyncAppendResponse(areq);
			break;
		default:
			/* If the node doesn't support async, caller messed up. */
			elog(ERROR, "unrecognized node type: %d",
/* …（同文件无关代码省略）… */
	areq->request_complete = false;
	areq->result = NULL;
}
