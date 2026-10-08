// AUTO-DRAFT from postgres/postgres PR #c993d9f7c8d4bcada0d5c0bd6ea32416d956fc8f
#include "access/tableam.h"
#include "access/xact.h"
#include "catalog/index.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_am_d.h"
#include "catalog/pg_collation.h"
/* …（同文件无关代码省略）… */
#include "miscadmin.h"
#include "parser/parse_coerce.h"
#include "parser/parse_relation.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/datum.h"
/* …（同文件无关代码省略）… */

	pk_rel = table_open(riinfo->pk_relid, RowShareLock);

	/*
	 * Advance the command counter so the check sees the effects of prior
	 * triggers in this statement, as SPI does when executing the query issued
