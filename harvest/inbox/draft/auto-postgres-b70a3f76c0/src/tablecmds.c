// AUTO-DRAFT from postgres/postgres PR #2bdbde8b3bd782ec18168e19b70ccaa945ab0177
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
				if (!tup)
					ereport(ERROR,
							errcode(ERRCODE_INVALID_TABLE_DEFINITION),
							errmsg("column \"%s\" of table \"%s\" is not marked NOT NULL",
								   strVal(column), get_rel_name(childrelid)));
				/* verify it's good enough */
				verifyNotNullPKCompatible(tup, strVal(column));
/* …（同文件无关代码省略）… */
static void
verifyNotNullPKCompatible(HeapTuple tuple, const char *colname)
{
	Form_pg_constraint conForm = (Form_pg_constraint) GETSTRUCT(tuple);

	if (conForm->contype != CONSTRAINT_NOTNULL)
		elog(ERROR, "constraint %u is not a not-null constraint", conForm->oid);

	/* a NO INHERIT constraint is no good */
	if (conForm->connoinherit)
		ereport(ERROR,
				errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				errmsg("cannot create primary key on column \"%s\"", colname),
		/*- translator: fourth %s is a constraint characteristic such as NOT VALID */
				errdetail("The constraint \"%s\" on column \"%s\" of table \"%s\", marked %s, is incompatible with a primary key.",
						  NameStr(conForm->conname), colname,
						  get_rel_name(conForm->conrelid), "NO INHERIT"),
				errhint("You might need to make the existing constraint inheritable using %s.",
						"ALTER TABLE ... ALTER CONSTRAINT ... INHERIT"));

	/* an unvalidated constraint is no good */
	if (!conForm->convalidated)
		ereport(ERROR,
				errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				errmsg("cannot create primary key on column \"%s\"", colname),
		/*- translator: fourth %s is a constraint characteristic such as NOT VALID */
				errdetail("The constraint \"%s\" on column \"%s\" of table \"%s\", marked %s, is incompatible with a primary key.",
						  NameStr(conForm->conname), colname,
						  get_rel_name(conForm->conrelid), "NOT VALID"),
				errhint("You might need to validate it using %s.",
						"ALTER TABLE ... VALIDATE CONSTRAINT"));
}
