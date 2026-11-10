// AUTO-DRAFT from postgres/postgres PR #edbb1d2dcb18d0706e644d3048a21e4475debd95
#include "executor/nodeBitmapIndexscan.h"
#include "executor/nodeCustom.h"
#include "executor/nodeForeignscan.h"
#include "executor/nodeHash.h"
#include "executor/nodeHashjoin.h"
#include "executor/nodeIncrementalSort.h"
/* …（同文件无关代码省略）… */
			/* even when not parallel-aware, for EXPLAIN ANALYZE */
			ExecMaterialEstimate((MaterialState *) planstate, e->pcxt);
			break;
		default:
			break;
	}
/* …（同文件无关代码省略）… */
			/* even when not parallel-aware, for EXPLAIN ANALYZE */
			ExecMaterialInitializeDSM((MaterialState *) planstate, d->pcxt);
			break;
		default:
			break;
	}
/* …（同文件无关代码省略）… */
		case T_IncrementalSortState:
		case T_MemoizeState:
		case T_MaterialState:
			/* these nodes have DSM state, but no reinitialization is required */
			break;

/* …（同文件无关代码省略）… */
		case T_MaterialState:
			ExecMaterialRetrieveInstrumentation((MaterialState *) planstate);
			break;
		case T_BitmapHeapScanState:
			ExecBitmapHeapRetrieveInstrumentation((BitmapHeapScanState *) planstate);
			break;
/* …（同文件无关代码省略）… */
			/* even when not parallel-aware, for EXPLAIN ANALYZE */
			ExecMaterialInitializeWorker((MaterialState *) planstate, pwcxt);
			break;
		default:
			break;
	}
