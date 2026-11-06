// AUTO-DRAFT from postgres/postgres PR #3c5a4dcaaa8d5bef429bd8143d1a6e9f92fa0b74
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

#include "access/relscan.h"
#include "access/tableam.h"
#include "access/xact.h"
#include "catalog/namespace.h"
#include "catalog/pg_am_d.h"
#include "catalog/pg_authid.h"
/* …（同文件无关代码省略）… */
		aclcheck_error(aclresult, get_relkind_objtype(rel->rd_rel->relkind),
					   RelationGetRelationName(rel));

	/* Scan the relation */
	scan = table_beginscan(rel, GetActiveSnapshot(), 0, NULL, SO_NONE);
	hscan = (HeapScanDesc) scan;
