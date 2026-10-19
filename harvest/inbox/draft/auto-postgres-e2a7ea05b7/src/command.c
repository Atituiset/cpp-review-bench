// AUTO-DRAFT from postgres/postgres PR #9d882c10bfb6f56a0a430872ef3040875535b06b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	char		vbuf[32];
	const char *server_version;
	char	   *service_name;
	char	   *service_file;

	/* get stuff from connection */
	pset.encoding = PQclientEncoding(pset.db);
/* …（同文件无关代码省略）… */
	if (service_name)
		pg_free(service_name);

	service_file = get_conninfo_value("servicefile");
	SetVariable(pset.vars, "SERVICEFILE", service_file);
	if (service_file)
		pg_free(service_file);

	/* this bit should match connection_warnings(): */
	/* Try to get full text form of version, might include "devel" etc */
	server_version = PQparameterStatus(pset.db, "server_version");
/* …（同文件无关代码省略）… */
{
	SetVariable(pset.vars, "DBNAME", NULL);
	SetVariable(pset.vars, "SERVICE", NULL);
	SetVariable(pset.vars, "SERVICEFILE", NULL);
	SetVariable(pset.vars, "USER", NULL);
	SetVariable(pset.vars, "HOST", NULL);
	SetVariable(pset.vars, "PORT", NULL);
