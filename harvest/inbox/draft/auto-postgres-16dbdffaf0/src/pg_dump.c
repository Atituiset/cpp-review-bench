// AUTO-DRAFT from postgres/postgres PR #35def3385998099f3e80e34f9d7bf6444af5bf1c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void
appendNamedArgument(PQExpBuffer out, Archive *fout, const char *argname,
					const char *argtype, const char *argval)
{
	appendPQExpBufferStr(out, ",\n\t");

	appendStringLiteralAH(out, argname, fout);
	appendPQExpBufferStr(out, ", ");

	appendStringLiteralAH(out, argval, fout);
	appendPQExpBuffer(out, "::%s", argtype);
}
/* …（同文件无关代码省略）… */
		 *   catalogs.
		 * - In v18 and older versions, ndistinct and dependencies have a
		 *   different format that needs translation.
		 * - In v14 and older versions, inherited does not exist.
		 * - In v11 and older versions, there is no pg_stats_ext, hence
		 *   the logic joins pg_statistic_ext and pg_namespace.
		 *---------
/* …（同文件无关代码省略）… */
		for (int i = 0; i < nstats; i++)
		{
			TableInfo  *tbinfo = statsextinfo->stattable;

			if (PQgetisnull(res, i, i_inherited))
				pg_fatal("inherited cannot be NULL");

			appendPQExpBufferStr(out,
								 "SELECT * FROM pg_catalog.pg_restore_extended_stats(\n");
			appendPQExpBuffer(out, "\t'version', '%d'::integer,\n",
/* …（同文件无关代码省略）… */
			appendStringLiteralAH(out, statsextinfo->dobj.namespace->dobj.name, fout);
			appendPQExpBufferStr(out, ",\n\t'statistics_name', ");
			appendStringLiteralAH(out, statsextinfo->dobj.name, fout);
			appendNamedArgument(out, fout, "inherited", "boolean",
								PQgetvalue(res, i, i_inherited));

			if (!PQgetisnull(res, i, i_ndistinct))
				appendNamedArgument(out, fout, "n_distinct", "pg_ndistinct",
