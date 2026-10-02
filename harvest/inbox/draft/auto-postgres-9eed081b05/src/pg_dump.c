// AUTO-DRAFT from postgres/postgres PR #80931d131a95ddcddc7b671d4fee7aceeb12a314
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct
{
	Oid			oid;			/* object OID */
	char		relkind;		/* object kind */
	RelFileNumber relfilenumber;	/* object filenode */
	Oid			toast_oid;		/* toast table OID */
	RelFileNumber toast_relfilenumber;	/* toast table filenode */
	Oid			toast_chunk_id_typoid;	/* type of chunk_id attribute */
	Oid			toast_index_oid;	/* toast table index OID */
	RelFileNumber toast_index_relfilenumber;	/* toast table index filenode */
} BinaryUpgradeClassOidItem;
/* …（同文件无关代码省略）… */
static void
append_depends_on_extension(Archive *fout,
							PQExpBuffer create,
							const DumpableObject *dobj,
							const char *catalog,
							const char *keyword,
							const char *objname)
{
	if (dobj->depends_on_ext)
	{
		char	   *nm;
		PGresult   *res;
		PQExpBuffer query;
		int			ntups;
		int			i_extname;
		int			i;

		/* dodge fmtId() non-reentrancy */
		nm = pg_strdup(objname);

		query = createPQExpBuffer();
		appendPQExpBuffer(query,
						  "SELECT e.extname "
						  "FROM pg_catalog.pg_depend d, pg_catalog.pg_extension e "
						  "WHERE d.refobjid = e.oid AND classid = '%s'::pg_catalog.regclass "
						  "AND objid = '%u'::pg_catalog.oid AND deptype = 'x' "
						  "AND refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass",
						  catalog,
						  dobj->catId.oid);
		res = ExecuteSqlQuery(fout, query->data, PGRES_TUPLES_OK);
		ntups = PQntuples(res);
		i_extname = PQfnumber(res, "extname");
		for (i = 0; i < ntups; i++)
		{
			appendPQExpBuffer(create, "\nALTER %s %s DEPENDS ON EXTENSION %s;",
							  keyword, nm,
							  fmtId(PQgetvalue(res, i, i_extname)));
		}

		PQclear(res);
		destroyPQExpBuffer(query);
		pg_free(nm);
	}
}
/* …（同文件无关代码省略）… */
static int
BinaryUpgradeClassOidItemCmp(const void *p1, const void *p2)
{
	BinaryUpgradeClassOidItem v1 = *((const BinaryUpgradeClassOidItem *) p1);
	BinaryUpgradeClassOidItem v2 = *((const BinaryUpgradeClassOidItem *) p2);

	return pg_cmp_u32(v1.oid, v2.oid);
}
/* …（同文件无关代码省略）… */
static void
binary_upgrade_set_pg_class_oids(Archive *fout,
								 PQExpBuffer upgrade_buffer, Oid pg_class_oid)
{
	BinaryUpgradeClassOidItem key = {0};
	BinaryUpgradeClassOidItem *entry;

	Assert(binaryUpgradeClassOids);

	/*
	 * Preserve the OID and relfilenumber of the table, table's index, table's
	 * toast table, toast table's chunk type and toast table's index if any.
	 *
	 * One complexity is that the current table definition might not require
	 * the creation of a TOAST table, but the old database might have a TOAST
	 * table that was created earlier, before some wide columns were dropped.
	 * By setting the TOAST oid we force creation of the TOAST heap and index
	 * by the new backend, so we can copy the files during binary upgrade
	 * without worrying about this case.
	 */
	key.oid = pg_class_oid;
	entry = bsearch(&key, binaryUpgradeClassOids, nbinaryUpgradeClassOids,
					sizeof(BinaryUpgradeClassOidItem),
					BinaryUpgradeClassOidItemCmp);

	appendPQExpBufferStr(upgrade_buffer,
						 "\n-- For binary upgrade, must preserve pg_class oids, toast chunk type oids and relfilenodes\n");

	if (entry->relkind != RELKIND_INDEX &&
		entry->relkind != RELKIND_PARTITIONED_INDEX)
	{
		appendPQExpBuffer(upgrade_buffer,
						  "SELECT pg_catalog.binary_upgrade_set_next_heap_pg_class_oid('%u'::pg_catalog.oid);\n",
						  pg_class_oid);

		/*
		 * Not every relation has storage. Also, in a pre-v12 database,
		 * partitioned tables have a relfilenumber, which should not be
		 * preserved when upgrading.
		 */
		if (RelFileNumberIsValid(entry->relfilenumber) &&
			entry->relkind != RELKIND_PARTITIONED_TABLE)
			appendPQExpBuffer(upgrade_buffer,
							  "SELECT pg_catalog.binary_upgrade_set_next_heap_relfilenode('%u'::pg_catalog.oid);\n",
							  entry->relfilenumber);

		/*
		 * In a pre-v12 database, partitioned tables might be marked as having
		 * toast tables, but we should ignore them if so.
		 */
		if (OidIsValid(entry->toast_oid) &&
			entry->relkind != RELKIND_PARTITIONED_TABLE)
		{
			appendPQExpBuffer(upgrade_buffer,
							  "SELECT pg_catalog.binary_upgrade_set_next_toast_pg_class_oid('%u'::pg_catalog.oid);\n",
							  entry->toast_oid);
			appendPQExpBuffer(upgrade_buffer,
							  "SELECT pg_catalog.binary_upgrade_set_next_toast_relfilenode('%u'::pg_catalog.oid);\n",
							  entry->toast_relfilenumber);
			appendPQExpBuffer(upgrade_buffer,
							  "SELECT pg_catalog.binary_upgrade_set_next_toast_chunk_id_typoid('%u'::pg_catalog.oid);\n",
							  entry->toast_chunk_id_typoid);

			/* every toast table has an index */
			appendPQExpBuffer(upgrade_buffer,
							  "SELECT pg_catalog.binary_upgrade_set_next_index_pg_class_oid('%u'::pg_catalog.oid);\n",
							  entry->toast_index_oid);
			appendPQExpBuffer(upgrade_buffer,
							  "SELECT pg_catalog.binary_upgrade_set_next_index_relfilenode('%u'::pg_catalog.oid);\n",
							  entry->toast_index_relfilenumber);
		}
	}
	else
	{
		/* Preserve the OID and relfilenumber of the index */
		appendPQExpBuffer(upgrade_buffer,
						  "SELECT pg_catalog.binary_upgrade_set_next_index_pg_class_oid('%u'::pg_catalog.oid);\n",
						  pg_class_oid);
		appendPQExpBuffer(upgrade_buffer,
						  "SELECT pg_catalog.binary_upgrade_set_next_index_relfilenode('%u'::pg_catalog.oid);\n",
						  entry->relfilenumber);
	}

	appendPQExpBufferChar(upgrade_buffer, '\n');
}
/* …（同文件无关代码省略）… */
	return NULL;				/* keep compiler quiet */
}

/*
 * dumpIndex
 *	  write out to fout a user-defined index
/* …（同文件无关代码省略）… */
	 */
	if (!is_constraint)
	{
		char	   *indstatcols = indxinfo->indstatcols;
		char	   *indstatvals = indxinfo->indstatvals;
		char	  **indstatcolsarray = NULL;
		char	  **indstatvalsarray = NULL;
		int			nstatcols = 0;
		int			nstatvals = 0;

		if (dopt->binary_upgrade)
			binary_upgrade_set_pg_class_oids(fout, q,
											 indxinfo->dobj.catId.oid);
/* …（同文件无关代码省略）… */
							  qindxname);
		}

		/*
		 * If the index has any statistics on some of its columns, generate
		 * the associated ALTER INDEX queries.
		 */
		if (strlen(indstatcols) != 0 || strlen(indstatvals) != 0)
		{
			int			j;

			if (!parsePGArray(indstatcols, &indstatcolsarray, &nstatcols))
				pg_fatal("could not parse index statistic columns");
			if (!parsePGArray(indstatvals, &indstatvalsarray, &nstatvals))
				pg_fatal("could not parse index statistic values");
			if (nstatcols != nstatvals)
				pg_fatal("mismatched number of columns and values for index statistics");

			for (j = 0; j < nstatcols; j++)
			{
				appendPQExpBuffer(q, "ALTER INDEX %s ", qqindxname);

				/*
				 * Note that this is a column number, so no quotes should be
				 * used.
				 */
				appendPQExpBuffer(q, "ALTER COLUMN %s ",
								  indstatcolsarray[j]);
				appendPQExpBuffer(q, "SET STATISTICS %s;\n",
								  indstatvalsarray[j]);
			}
		}

		/* Indexes can depend on extensions */
		append_depends_on_extension(fout, q, &indxinfo->dobj,
/* …（同文件无关代码省略）… */
									  .section = SECTION_POST_DATA,
									  .createStmt = q->data,
									  .dropStmt = delq->data));

		free(indstatcolsarray);
		free(indstatvalsarray);
	}

	/* Dump Index Comments */
/* …（同文件无关代码省略）… */
							  fmtId(indxinfo->dobj.name));
		}

		/* If the index defines identity, we need to record that. */
		if (indxinfo->indisreplident)
		{
