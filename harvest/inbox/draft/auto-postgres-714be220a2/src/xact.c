// AUTO-DRAFT from postgres/postgres PR #425daf545d9146e008220ae0b21415982220cd3f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
					  s->parent->subTransactionId);
	AtEOSubXact_HashTables(true, s->nestingLevel);
	AtEOSubXact_PgStat(true, s->nestingLevel);
	AtEOSubXact_RI(true, s->subTransactionId, s->parent->subTransactionId);
	AtSubCommit_Snapshot(s->nestingLevel);

	/*
/* …（同文件无关代码省略）… */
						  s->parent->subTransactionId);
		AtEOSubXact_HashTables(false, s->nestingLevel);
		AtEOSubXact_PgStat(false, s->nestingLevel);
		AtEOSubXact_RI(false, s->subTransactionId, s->parent->subTransactionId);
		AtSubAbort_Snapshot(s->nestingLevel);
	}
