// AUTO-DRAFT from postgres/postgres PR #a241bc4fedefae3c0b0952c0a9d3a28d7747bfe3
#include "executor/nodeIncrementalSort.h"
#include "executor/nodeIndexonlyscan.h"
#include "executor/nodeIndexscan.h"
#include "executor/nodeMemoize.h"
#include "executor/nodeSeqscan.h"
#include "executor/nodeSort.h"
/* …（同文件无关代码省略）… */
			/* even when not parallel-aware, for EXPLAIN ANALYZE */
			ExecMemoizeEstimate((MemoizeState *) planstate, e->pcxt);
			break;
		default:
			break;
	}
/* …（同文件无关代码省略）… */
			/* even when not parallel-aware, for EXPLAIN ANALYZE */
			ExecMemoizeInitializeDSM((MemoizeState *) planstate, d->pcxt);
			break;
		default:
			break;
	}
/* …（同文件无关代码省略）… */
		case T_SortState:
		case T_IncrementalSortState:
		case T_MemoizeState:
			/* these nodes have DSM state, but no reinitialization is required */
			break;

/* …（同文件无关代码省略）… */
		case T_MemoizeState:
			ExecMemoizeRetrieveInstrumentation((MemoizeState *) planstate);
			break;
		case T_BitmapHeapScanState:
			ExecBitmapHeapRetrieveInstrumentation((BitmapHeapScanState *) planstate);
			break;
/* …（同文件无关代码省略）… */
			/* even when not parallel-aware, for EXPLAIN ANALYZE */
			ExecMemoizeInitializeWorker((MemoizeState *) planstate, pwcxt);
			break;
		default:
			break;
	}
