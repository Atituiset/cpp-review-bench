// AUTO-DRAFT from postgres/postgres PR #126976a8ffcfa47f40d3df09243ebf4606001912
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

 *								outer joins
 *		hj_NullInnerTupleSlot	prepared null tuple for left/full outer joins
 *		hj_NullOuterTupleStore	tuplestore holding outer tuples that have
 *								null join keys (but must be emitted anyway)
 *		hj_FirstOuterTupleSlot	first tuple retrieved from outer plan
 *		hj_JoinState			current state of ExecHashJoin state machine
 *		hj_KeepNullTuples		true to keep outer tuples with null join keys
