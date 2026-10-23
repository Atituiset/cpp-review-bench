// AUTO-DRAFT from postgres/postgres PR #b6dae498dfa60c9d15abf791d51434e8d23e5cae
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct
{
	ParseState *pstate;			/* overall parser state */
	const char *stmtType;		/* "CREATE [FOREIGN] TABLE" or "ALTER TABLE" */
	RangeVar   *relation;		/* relation to create */
	Relation	rel;			/* opened/locked rel, if ALTER */
	List	   *inhRelations;	/* relations to inherit from */
	bool		isforeign;		/* true if CREATE/ALTER FOREIGN TABLE */
	bool		isalter;		/* true if altering existing table */
	List	   *columns;		/* ColumnDef items */
	List	   *ckconstraints;	/* CHECK constraints */
	List	   *nnconstraints;	/* NOT NULL constraints */
	List	   *fkconstraints;	/* FOREIGN KEY constraints */
	List	   *ixconstraints;	/* index-creating constraints */
	List	   *likeclauses;	/* LIKE clauses that need post-processing */
	List	   *blist;			/* "before list" of things to do before
								 * creating the table */
	List	   *alist;			/* "after list" of things to do after creating
								 * the table */
	IndexStmt  *pkey;			/* PRIMARY KEY index, if any */
	bool		ispartitioned;	/* true if table is partitioned */
	PartitionBoundSpec *partbound;	/* transformed FOR VALUES */
	bool		ofType;			/* true if statement contains OF typename */
} CreateStmtContext;
/* …（同文件无关代码省略）… */
typedef struct
{
	ParseState *pstate;			/* overall parse state */
	const char *schemaname;		/* name of schema */
	List	   *sequences;		/* CREATE SEQUENCE items */
	List	   *tables;			/* CREATE TABLE items */
	List	   *views;			/* CREATE VIEW items */
	List	   *indexes;		/* CREATE INDEX items */
	List	   *triggers;		/* CREATE TRIGGER items */
	List	   *grants;			/* GRANT items */
	List	   *foreign_keys;	/* generated ALTER ADD FOREIGN KEY items */
} CreateSchemaStmtContext;
/* …（同文件无关代码省略）… */
static void transformConstraintAttrs(ParseState *pstate,
									 List *constraintList);
static void transformColumnType(CreateStmtContext *cxt, ColumnDef *column);
static void checkSchemaNameRV(CreateSchemaStmtContext *cxt, RangeVar *relation);
static CreateStmt *transformCreateSchemaCreateTable(ParseState *pstate,
													CreateStmt *stmt,
													List **fk_elements);
/* …（同文件无关代码省略）… */
static void
transformConstraintAttrs(ParseState *pstate, List *constraintList)
{
	Constraint *lastprimarycon = NULL;
	bool		saw_deferrability = false;
	bool		saw_initially = false;
	bool		saw_enforced = false;
	ListCell   *clist;

#define SUPPORTS_ATTRS(node)				\
	((node) != NULL &&						\
	 ((node)->contype == CONSTR_PRIMARY ||	\
	  (node)->contype == CONSTR_UNIQUE ||	\
	  (node)->contype == CONSTR_EXCLUSION || \
	  (node)->contype == CONSTR_FOREIGN))

	foreach(clist, constraintList)
	{
		Constraint *con = (Constraint *) lfirst(clist);

		if (!IsA(con, Constraint))
			elog(ERROR, "unrecognized node type: %d",
				 (int) nodeTag(con));
		switch (con->contype)
		{
			case CONSTR_ATTR_DEFERRABLE:
				if (!SUPPORTS_ATTRS(lastprimarycon))
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("misplaced DEFERRABLE clause"),
							 parser_errposition(pstate, con->location)));
				if (saw_deferrability)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("multiple DEFERRABLE/NOT DEFERRABLE clauses not allowed"),
							 parser_errposition(pstate, con->location)));
				saw_deferrability = true;
				lastprimarycon->deferrable = true;
				break;

			case CONSTR_ATTR_NOT_DEFERRABLE:
				if (!SUPPORTS_ATTRS(lastprimarycon))
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("misplaced NOT DEFERRABLE clause"),
							 parser_errposition(pstate, con->location)));
				if (saw_deferrability)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("multiple DEFERRABLE/NOT DEFERRABLE clauses not allowed"),
							 parser_errposition(pstate, con->location)));
				saw_deferrability = true;
				lastprimarycon->deferrable = false;
				if (saw_initially &&
					lastprimarycon->initdeferred)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("constraint declared INITIALLY DEFERRED must be DEFERRABLE"),
							 parser_errposition(pstate, con->location)));
				break;

			case CONSTR_ATTR_DEFERRED:
				if (!SUPPORTS_ATTRS(lastprimarycon))
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("misplaced INITIALLY DEFERRED clause"),
							 parser_errposition(pstate, con->location)));
				if (saw_initially)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("multiple INITIALLY IMMEDIATE/DEFERRED clauses not allowed"),
							 parser_errposition(pstate, con->location)));
				saw_initially = true;
				lastprimarycon->initdeferred = true;

				/*
				 * If only INITIALLY DEFERRED appears, assume DEFERRABLE
				 */
				if (!saw_deferrability)
					lastprimarycon->deferrable = true;
				else if (!lastprimarycon->deferrable)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("constraint declared INITIALLY DEFERRED must be DEFERRABLE"),
							 parser_errposition(pstate, con->location)));
				break;

			case CONSTR_ATTR_IMMEDIATE:
				if (!SUPPORTS_ATTRS(lastprimarycon))
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("misplaced INITIALLY IMMEDIATE clause"),
							 parser_errposition(pstate, con->location)));
				if (saw_initially)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("multiple INITIALLY IMMEDIATE/DEFERRED clauses not allowed"),
							 parser_errposition(pstate, con->location)));
				saw_initially = true;
				lastprimarycon->initdeferred = false;
				break;

			case CONSTR_ATTR_ENFORCED:
				if (lastprimarycon == NULL ||
					(lastprimarycon->contype != CONSTR_CHECK &&
					 lastprimarycon->contype != CONSTR_FOREIGN))
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("misplaced ENFORCED clause"),
							 parser_errposition(pstate, con->location)));
				if (saw_enforced)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("multiple ENFORCED/NOT ENFORCED clauses not allowed"),
							 parser_errposition(pstate, con->location)));
				saw_enforced = true;
				lastprimarycon->is_enforced = true;
				break;

			case CONSTR_ATTR_NOT_ENFORCED:
				if (lastprimarycon == NULL ||
					(lastprimarycon->contype != CONSTR_CHECK &&
					 lastprimarycon->contype != CONSTR_FOREIGN))
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("misplaced NOT ENFORCED clause"),
							 parser_errposition(pstate, con->location)));
				if (saw_enforced)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("multiple ENFORCED/NOT ENFORCED clauses not allowed"),
							 parser_errposition(pstate, con->location)));
				saw_enforced = true;
				lastprimarycon->is_enforced = false;

				/* A NOT ENFORCED constraint must be marked as invalid. */
				lastprimarycon->skip_validation = true;
				lastprimarycon->initially_valid = false;
				break;

			default:
				/* Otherwise it's not an attribute */
				lastprimarycon = con;
				/* reset flags for new primary node */
				saw_deferrability = false;
				saw_initially = false;
				saw_enforced = false;
				break;
		}
	}
}
/* …（同文件无关代码省略）… */
static void
transformColumnType(CreateStmtContext *cxt, ColumnDef *column)
{
	/*
	 * All we really need to do here is verify that the type is valid,
	 * including any collation spec that might be present.
	 */
	Type		ctype = typenameType(cxt->pstate, column->typeName, NULL);

	if (column->collClause)
	{
		Form_pg_type typtup = (Form_pg_type) GETSTRUCT(ctype);

		LookupCollation(cxt->pstate,
						column->collClause->collname,
						column->collClause->location);
		/* Complain if COLLATE is applied to an uncollatable type */
		if (!OidIsValid(typtup->typcollation))
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("collations are not supported by type %s",
							format_type_be(typtup->oid)),
					 parser_errposition(cxt->pstate,
										column->collClause->location)));
	}

	ReleaseSysCache(ctype);
}
/* …（同文件无关代码省略）… */
 * transformCreateSchemaStmtElements -
 *	  analyzes the elements of a CREATE SCHEMA statement
 *
 * This presently has two responsibilities.  We verify that no subcommands
 * are trying to create objects outside the new schema.  We also attempt to
 * re-order the subcommands such that there are no forward references
 * (e.g. GRANT to a table created later in the list).  Note that the logic
/* …（同文件无关代码省略）… */
	List	   *result;
	ListCell   *elements;

	cxt.pstate = pstate;
	cxt.schemaname = schemaName;
	cxt.sequences = NIL;
/* …（同文件无关代码省略）… */
	cxt.foreign_keys = NIL;

	/*
	 * Run through each schema element in the schema element list.  Check
	 * target schema names, separate statements by type, and do preliminary
	 * analysis.
	 */
	foreach(elements, schemaElts)
	{
/* …（同文件无关代码省略）… */
				{
					CreateSeqStmt *elp = (CreateSeqStmt *) element;

					checkSchemaNameRV(&cxt, elp->sequence);
					cxt.sequences = lappend(cxt.sequences, element);
				}
				break;
/* …（同文件无关代码省略）… */
				{
					CreateStmt *elp = (CreateStmt *) element;

					checkSchemaNameRV(&cxt, elp->relation);
					/* Pull out any foreign key clauses, add to fk_elements */
					elp = transformCreateSchemaCreateTable(pstate,
														   elp,
/* …（同文件无关代码省略）… */
				{
					ViewStmt   *elp = (ViewStmt *) element;

					checkSchemaNameRV(&cxt, elp->view);

					/*
					 * XXX todo: deal with references between views
/* …（同文件无关代码省略）… */
				{
					IndexStmt  *elp = (IndexStmt *) element;

					checkSchemaNameRV(&cxt, elp->relation);
					cxt.indexes = lappend(cxt.indexes, element);
				}
				break;
/* …（同文件无关代码省略）… */
				{
					CreateTrigStmt *elp = (CreateTrigStmt *) element;

					checkSchemaNameRV(&cxt, elp->relation);
					cxt.triggers = lappend(cxt.triggers, element);
				}
				break;
/* …（同文件无关代码省略）… */
}

/*
 * checkSchemaNameRV
 *		Check schema name in an element of a CREATE SCHEMA command,
 *		where the element's name is given by a RangeVar
 *
 * It's okay if the command doesn't specify a target schema name, because
 * CreateSchemaCommand will set up the default creation schema to be the
 * new schema.  But if a target schema name is given, it had better match.
 * We also have to check that the command doesn't say CREATE TEMP, since
 * that would likewise put the object into the wrong schema.
 */
static void
checkSchemaNameRV(CreateSchemaStmtContext *cxt, RangeVar *relation)
{
	if (relation->schemaname != NULL &&
		strcmp(cxt->schemaname, relation->schemaname) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_SCHEMA_DEFINITION),
				 errmsg("CREATE specifies a schema (%s) "
/* …（同文件无关代码省略）… */
static CreateStmt *
transformCreateSchemaCreateTable(ParseState *pstate,
								 CreateStmt *stmt,
								 List **fk_elements)
{
	CreateStmt *newstmt;
	List	   *newElts = NIL;
	ListCell   *lc;

	/*
	 * Flat-copy the CreateStmt node, allowing us to replace its tableElts
	 * list without damaging the input data structure.  Most sub-nodes will be
	 * shared with the input, though.
	 */
	newstmt = makeNode(CreateStmt);
	memcpy(newstmt, stmt, sizeof(CreateStmt));

	/* Scan for foreign-key constraints */
	foreach(lc, stmt->tableElts)
	{
		Node	   *element = lfirst(lc);
		AlterTableStmt *alterstmt;
		AlterTableCmd *altercmd;

		if (IsA(element, Constraint))
		{
			Constraint *constr = (Constraint *) element;

			if (constr->contype != CONSTR_FOREIGN)
			{
				/* Other constraint types pass through unchanged */
				newElts = lappend(newElts, constr);
				continue;
			}

			/* Make it into an ALTER TABLE ADD CONSTRAINT command */
			altercmd = makeNode(AlterTableCmd);
			altercmd->subtype = AT_AddConstraint;
			altercmd->name = NULL;
			altercmd->def = (Node *) copyObject(constr);

			alterstmt = makeNode(AlterTableStmt);
			alterstmt->relation = copyObject(stmt->relation);
			alterstmt->cmds = list_make1(altercmd);
			alterstmt->objtype = OBJECT_TABLE;

			*fk_elements = lappend(*fk_elements, alterstmt);
		}
		else if (IsA(element, ColumnDef))
		{
			ColumnDef  *entry = (ColumnDef *) element;
			ColumnDef  *newentry;
			List	   *entryconstraints;
			bool		afterFK = false;

			/*
			 * We must preprocess the list of column constraints to attach
			 * attributes such as DEFERRED to the appropriate constraint node.
			 * Do this on a copy.  (But execution of the CreateStmt will run
			 * transformConstraintAttrs on the copy, so we are nonetheless
			 * relying on transformConstraintAttrs to be idempotent.)
			 */
			entryconstraints = copyObject(entry->constraints);
			transformConstraintAttrs(pstate, entryconstraints);

			/* Scan the column constraints ... */
			foreach_node(Constraint, colconstr, entryconstraints)
			{
				switch (colconstr->contype)
				{
					case CONSTR_FOREIGN:
						/* colconstr is already a copy, OK to modify */
						colconstr->fk_attrs = list_make1(makeString(entry->colname));

						/* Make it into an ALTER TABLE ADD CONSTRAINT command */
						altercmd = makeNode(AlterTableCmd);
						altercmd->subtype = AT_AddConstraint;
						altercmd->name = NULL;
						altercmd->def = (Node *) colconstr;

						alterstmt = makeNode(AlterTableStmt);
						alterstmt->relation = copyObject(stmt->relation);
						alterstmt->cmds = list_make1(altercmd);
						alterstmt->objtype = OBJECT_TABLE;

						*fk_elements = lappend(*fk_elements, alterstmt);

						/* Remove the Constraint node from entryconstraints */
						entryconstraints =
							foreach_delete_current(entryconstraints, colconstr);

						/*
						 * Immediately-following attribute constraints should
						 * be dropped, too.
						 */
						afterFK = true;
						break;

						/*
						 * Column constraint lists separate a Constraint node
						 * from its attributes (e.g. NOT ENFORCED); so a
						 * column-level foreign key constraint may be
						 * represented by multiple Constraint nodes.  After
						 * transformConstraintAttrs, the foreign key
						 * Constraint node contains all required information,
						 * making it okay to put into *fk_elements as a
						 * stand-alone Constraint.  But since we removed the
						 * foreign key Constraint node from entryconstraints,
						 * we must remove any dependent attribute nodes too,
						 * else the later re-execution of
						 * transformConstraintAttrs will misbehave.
						 */
					case CONSTR_ATTR_DEFERRABLE:
					case CONSTR_ATTR_NOT_DEFERRABLE:
					case CONSTR_ATTR_DEFERRED:
					case CONSTR_ATTR_IMMEDIATE:
					case CONSTR_ATTR_ENFORCED:
					case CONSTR_ATTR_NOT_ENFORCED:
						if (afterFK)
							entryconstraints =
								foreach_delete_current(entryconstraints,
													   colconstr);
						break;

					default:
						/* Any following constraint attributes are unrelated */
						afterFK = false;
						break;
				}
			}

			/* Now make a modified ColumnDef to put into newElts */
			newentry = makeNode(ColumnDef);
			memcpy(newentry, entry, sizeof(ColumnDef));
			newentry->constraints = entryconstraints;
			newElts = lappend(newElts, newentry);
		}
		else
		{
			/* Other node types pass through unchanged */
			newElts = lappend(newElts, element);
		}
	}

	newstmt->tableElts = newElts;
	return newstmt;
}
