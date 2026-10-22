// AUTO-DRAFT from postgres/postgres PR #fded90eec3ebab14a327a3c40263e2366eb3c42a
#include "access/sysattr.h"
#include "access/table.h"
#include "access/tableam.h"
#include "catalog/namespace.h"
#include "catalog/pg_type.h"
#include "common/hashfn.h"
/* …（同文件无关代码省略）… */
					   get_namespace_name(RelationGetNamespace(rel)),
					   RelationGetRelationName(rel)));

	ItemPointerCopy(tid, result);

	snapshot = RegisterSnapshot(GetLatestSnapshot());
