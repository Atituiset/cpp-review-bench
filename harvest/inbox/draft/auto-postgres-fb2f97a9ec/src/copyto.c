// AUTO-DRAFT from postgres/postgres PR #1e707d32564497ed2478f4186b90d0821719957b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "access/table.h"
#include "access/tableam.h"
#include "access/tupconvert.h"
#include "catalog/pg_inherits.h"
#include "commands/copyapi.h"
#include "commands/progress.h"
/* …（同文件无关代码省略）… */
typedef enum CopyDest
{
	COPY_FILE,					/* to file (or a piped program) */
	COPY_FRONTEND,				/* to frontend */
	COPY_CALLBACK,				/* to callback function */
} CopyDest;
/* …（同文件无关代码省略）… */
typedef struct CopyToStateData
{
	/* format-specific routines */
	const CopyToRoutine *routine;

	/* low-level state data */
	CopyDest	copy_dest;		/* type of copy source/destination */
	FILE	   *copy_file;		/* used if copy_dest == COPY_FILE */
	StringInfo	fe_msgbuf;		/* used for all dests during COPY TO */

	int			file_encoding;	/* file or remote side's character encoding */
	bool		need_transcoding;	/* file encoding diff from server? */
	bool		encoding_embeds_ascii;	/* ASCII can be non-first byte? */

	/* parameters from the COPY command */
	Relation	rel;			/* relation to copy to */
	QueryDesc  *queryDesc;		/* executable query to copy from */
	List	   *attnumlist;		/* integer list of attnums to copy */
	char	   *filename;		/* filename, or NULL for STDOUT */
	bool		is_program;		/* is 'filename' a program to popen? */
	bool		json_row_delim_needed;	/* need delimiter before next row */
	StringInfo	json_buf;		/* reusable buffer for JSON output,
								 * initialized in BeginCopyTo */
	TupleDesc	tupDesc;		/* Descriptor for JSON output; for a column
								 * list this is a projected descriptor */
	Datum	   *json_projvalues;	/* pre-allocated projection values, or
									 * NULL */
	bool	   *json_projnulls; /* pre-allocated projection nulls, or NULL */
	copy_data_dest_cb data_dest_cb; /* function for writing data */

	CopyFormatOptions opts;
	Node	   *whereClause;	/* WHERE condition (or NULL) */
	List	   *partitions;		/* OID list of partitions to copy data from */

	/*
	 * Working state
	 */
	MemoryContext copycontext;	/* per-copy execution context */

	FmgrInfo   *out_functions;	/* lookup info for output functions */
	MemoryContext rowcontext;	/* per-row evaluation context */
	uint64		bytes_processed;	/* number of bytes processed so far */
} CopyToStateData;
/* …（同文件无关代码省略）… */
							errhint("Try the COPY (SELECT ...) TO variant."));
				}

				/* Exclude tables with no data */
				if (RELKIND_HAS_PARTITIONS(relkind))
					children = foreach_delete_current(children, child);
/* …（同文件无关代码省略）… */
							RelationGetRelationName(rel))));
	}


	/* Allocate workspace and zero all fields */
	cstate = palloc0_object(CopyToStateData);
