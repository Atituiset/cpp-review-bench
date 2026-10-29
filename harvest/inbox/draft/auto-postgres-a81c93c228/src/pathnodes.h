// AUTO-DRAFT from postgres/postgres PR #f400289946f7d0e66a4169cf4aa56079f1422a2b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <time.h>

typedef struct QualCost
{
	Cost		startup;		/* one-time cost */
	Cost		per_tuple;		/* per-evaluation cost */
} QualCost;
/* …（同文件无关代码省略）… */
	/* eval cost of clause; -1 if not yet set */
	QualCost	eval_cost pg_node_attr(equal_ignore);

	/* selectivity for "normal" (JOIN_INNER) semantics; -1 if not yet set */
	Selectivity norm_selec pg_node_attr(equal_ignore);
	/* selectivity for outer join semantics; -1 if not yet set */
	Selectivity outer_selec pg_node_attr(equal_ignore);
