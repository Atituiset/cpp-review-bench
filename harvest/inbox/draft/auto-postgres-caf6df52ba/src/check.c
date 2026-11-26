// AUTO-DRAFT from postgres/postgres PR #388f2db2994e216f15f004c4addbe4851871f954
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	 * parents columns have, but schema restore fails for them.  Verify there
	 * are none, iff applicable.
	 */
	if (GET_MAJOR_VERSION(old_cluster.major_version) <= 1800)
		check_for_not_null_inheritance(&old_cluster);

	/*
/* …（同文件无关代码省略）… */
static void
process_inconsistent_notnull(DbInfo *dbinfo, PGresult *res, void *arg)
{
	UpgradeTaskReport *report = (UpgradeTaskReport *) arg;
	int			ntups = PQntuples(res);
	int			i_nspname = PQfnumber(res, "nspname");
	int			i_relname = PQfnumber(res, "relname");
	int			i_attname = PQfnumber(res, "attname");

	if (ntups == 0)
		return;

	if (report->file == NULL &&
		(report->file = fopen_priv(report->path, "w")) == NULL)
		pg_fatal("could not open file \"%s\": %m", report->path);

	fprintf(report->file, "In database: %s\n", dbinfo->db_name);

	for (int rowno = 0; rowno < ntups; rowno++)
	{
		fprintf(report->file, "  %s.%s.%s\n",
				PQgetvalue(res, rowno, i_nspname),
				PQgetvalue(res, rowno, i_relname),
				PQgetvalue(res, rowno, i_attname));
	}
}
/* …（同文件无关代码省略）… */
static void
check_for_not_null_inheritance(ClusterInfo *cluster)
{
	UpgradeTaskReport report;
	UpgradeTask *task;
	const char *query;

	prep_status("Checking for not-null constraint inconsistencies");

	report.file = NULL;
	snprintf(report.path, sizeof(report.path), "%s/%s",
			 log_opts.basedir,
			 "not_null_inconsistent_columns.txt");

	query = "SELECT nspname, cc.relname, ac.attname "
		"FROM pg_catalog.pg_inherits i, pg_catalog.pg_attribute ac, "
		"     pg_catalog.pg_attribute ap, pg_catalog.pg_class cc, "
		"     pg_catalog.pg_namespace nc "
		"WHERE cc.oid = ac.attrelid AND i.inhrelid = ac.attrelid "
		"      AND i.inhparent = ap.attrelid AND ac.attname = ap.attname "
		"      AND cc.relnamespace = nc.oid "
		"      AND ap.attnum > 0 and ap.attnotnull AND NOT ac.attnotnull";

	task = upgrade_task_create();
	upgrade_task_add_step(task, query,
						  process_inconsistent_notnull,
						  true, &report);
	upgrade_task_run(task, cluster);
	upgrade_task_free(task);

	if (report.file)
	{
		fclose(report.file);
		pg_log(PG_REPORT, "fatal");
		pg_fatal("Your installation contains inconsistent NOT NULL constraints.\n"
				 "If the parent column(s) are NOT NULL, then the child column must\n"
				 "also be marked NOT NULL, or the upgrade will fail.\n"
				 "You can fix this by running\n"
				 "    ALTER TABLE tablename ALTER column SET NOT NULL;\n"
				 "on each column listed in the file:\n"
				 "    %s", report.path);
	}
	else
		check_ok();
}
