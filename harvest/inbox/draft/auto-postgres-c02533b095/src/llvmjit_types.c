// AUTO-DRAFT from postgres/postgres PR #0607aa61090ac1b574f8bafe854c684195b31e16
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>

 * List of types needed for JITing. These have to be non-static, otherwise
 * clang/LLVM will omit them.  As this file will never be linked into
 * anything, that's harmless.
 */
PGFunction	TypePGFunction;
size_t		TypeSizeT;
Datum		TypeDatum;
/* …（同文件无关代码省略）… */
PlanState	StructPlanState;
MinimalTupleData StructMinimalTupleData;


/*
 * To determine which attributes functions need to have (depends e.g. on
