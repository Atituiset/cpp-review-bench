// AUTO-DRAFT from postgres/postgres PR #debfb3c8a8c93e7bcc3638523b7ffc0846cc2db8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>

typedef struct
{
	AttrNumber	local_attnum;	/* attribute number of local column */
	char	   *local_attname;	/* attribute name of local column */
	char	   *remote_attname; /* attribute name of remote column */
	int			res_index;		/* index of row in attribute stats result */
} RemoteAttributeMapping;
/* …（同文件无关代码省略）… */
/*
 * Match local columns to result set rows.
 *
 * As the result set consists of the attribute stats for some/all of distinct
 * mapped remote columns in the RemoteAttributeMapping, every entry in it
 * should have at most one match in the result set; which is also ordered by
 * remote_attname, so we find such pairs by doing a merge join.
 *
 * Returns true if every entry in it has a match, and false if not.
 */
/* …（同文件无关代码省略）… */
		row += 1;

		/*
		 * If the attname in the entry is less than that in the next stats
		 * row, it means the stats for the entry are missing.
		 */
		if (strcmp(remattrmap[mapidx].remote_attname,
				   PQgetvalue(res, row, ATTSTATS_ATTNAME)) < 0)
/* …（同文件无关代码省略）… */
}

/*
 * Import fetched statistics into the local statistics tables.
 */
static bool
import_fetched_statistics(Relation relation,
/* …（同文件无关代码省略）… */
						  const RemoteAttributeMapping *remattrmap,
						  int attrcnt)
{
	NullableDatum args[ATTSTATS_NUM_FIELDS];

	/* Set the 'version' parameter, which is common to both statistics. */
