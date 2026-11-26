// AUTO-DRAFT from postgres/postgres PR #b8bf28800559a9ad71360fb2384d4b9981d24fa8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

 * IsConflictLogTableClass
 *		True iff pg_class tuple represents a Conflict Log Table.
 *
 *		Does not perform any catalog accesses.
 */
bool
IsConflictLogTableClass(Form_pg_class reltuple)
{
	Oid			relnamespace = reltuple->relnamespace;

	return IsConflictLogTableNamespace(relnamespace);
}
/* …（同文件无关代码省略）… */
 *		True iff namespace is pg_conflict.
 *
 *		Does not perform any catalog accesses.
 */
bool
IsConflictLogTableNamespace(Oid namespaceId)
{
	return namespaceId == PG_CONFLICT_NAMESPACE;
}
