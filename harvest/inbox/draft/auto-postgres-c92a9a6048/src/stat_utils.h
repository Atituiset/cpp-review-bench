// AUTO-DRAFT from postgres/postgres PR #2656f664616dac41e99495b39e2bc3bc1d81b1e7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
typedef struct TypeCacheEntry TypeCacheEntry;
/* …（同文件无关代码省略）… */
							 Datum stavalues, bool stavalues_isnull);

extern Datum statatt_build_stavalues(const char *staname, FmgrInfo *array_in, Datum d,
									 Oid typid, int32 typmod, bool *ok);
extern bool statatt_get_elem_type(TypeCacheEntry *basetypcache,
								  Oid *elemtypid, Oid *elem_eq_opr);
extern bool statatt_get_range_type(TypeCacheEntry *basetypcache,
