// AUTO-DRAFT from postgres/postgres PR #b58a9a97fe783c8ce897fa1a5da23c59234aa6bf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "libpq/pqformat.h"
/* …（同文件无关代码省略）… */

/*
 * Is the given type immutable when coming out of a JSONB context?
 */
bool
to_jsonb_is_immutable(Oid typoid)
{
	bool		has_mutable = false;

	json_check_mutability(typoid, true, &has_mutable);
	return !has_mutable;
}

/*
