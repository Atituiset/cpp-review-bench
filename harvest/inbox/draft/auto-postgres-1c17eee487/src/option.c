// AUTO-DRAFT from postgres/postgres PR #1d4324a2a302514a24908435dede693cff81fa37
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct PgFdwOption
{
	const char *keyword;
	Oid			optcontext;		/* OID of catalog in which option may appear */
	bool		is_libpq_opt;	/* true if it's used in libpq */
} PgFdwOption;
/* …（同文件无关代码省略）… */
	int			num_libpq_opts;
	PQconninfoOption *libpq_options;
	PQconninfoOption *lopt;
	PgFdwOption *popt;

	/* non-libpq FDW-specific FDW options */
/* …（同文件无关代码省略）… */
	 * Construct an array which consists of all valid options for
	 * postgres_fdw, by appending FDW-specific options to libpq options.
	 */
	postgres_fdw_options = (PgFdwOption *)
		MemoryContextAlloc(TopMemoryContext,
						   sizeof(PgFdwOption) * num_libpq_opts +
						   sizeof(non_libpq_options));

	popt = postgres_fdw_options;
	for (lopt = libpq_options; lopt->keyword; lopt++)
	{
		/* Hide debug options, as well as settings we override internally. */
/* …（同文件无关代码省略）… */

	/* Append FDW-specific options and dummy terminator. */
	memcpy(popt, non_libpq_options, sizeof(non_libpq_options));
}

/*
