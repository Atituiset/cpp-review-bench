// AUTO-DRAFT from postgres/postgres PR #af090508018be5ace443ba6e2814500c10842a4d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	 * Do not lock the children until they're processed.  Note that we do hold
	 * a lock on the parent partitioned table.
	 */
	inhoids = find_all_inheritors(relid, NoLock, NULL);
	foreach_oid(child_oid, inhoids)
	{
		Oid			table_oid,
