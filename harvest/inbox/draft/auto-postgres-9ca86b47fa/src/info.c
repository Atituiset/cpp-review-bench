// AUTO-DRAFT from postgres/postgres PR #32dd742b075e999fa358570bc13778b93f8025d4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	int			i_datcollate;
	int			i_datctype;
	int			i_datlocale;

	if (GET_MAJOR_VERSION(cluster->major_version) >= 1700)
		dbres = executeQueryOrDie(conn,
								  "SELECT encoding, datlocprovider, "
								  "       datcollate, datctype, datlocale "
								  "FROM	pg_catalog.pg_database "
								  "WHERE datname='template0'");
	else if (GET_MAJOR_VERSION(cluster->major_version) >= 1500)
		dbres = executeQueryOrDie(conn,
								  "SELECT encoding, datlocprovider, "
								  "       datcollate, datctype, daticulocale AS datlocale "
								  "FROM	pg_catalog.pg_database "
								  "WHERE datname='template0'");
	else
		dbres = executeQueryOrDie(conn,
								  "SELECT encoding, 'c' AS datlocprovider, "
								  "       datcollate, datctype, NULL AS datlocale "
								  "FROM	pg_catalog.pg_database "
								  "WHERE datname='template0'");

/* …（同文件无关代码省略）… */
	i_datcollate = PQfnumber(dbres, "datcollate");
	i_datctype = PQfnumber(dbres, "datctype");
	i_datlocale = PQfnumber(dbres, "datlocale");

	locale->db_encoding = atoi(PQgetvalue(dbres, 0, i_datencoding));
	locale->db_collprovider = PQgetvalue(dbres, 0, i_datlocprovider)[0];
/* …（同文件无关代码省略）… */
		locale->db_locale = NULL;
	else
		locale->db_locale = pg_strdup(PQgetvalue(dbres, 0, i_datlocale));

	cluster->template0 = locale;
