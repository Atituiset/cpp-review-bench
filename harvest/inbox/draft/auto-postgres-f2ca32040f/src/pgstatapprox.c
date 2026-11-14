// AUTO-DRAFT from postgres/postgres PR #206e71935e1ed86e6116b118d7b77e78efcff8b8
typedef struct output_type
{
	uint64		table_len;
	double		scanned_percent;
	uint64		tuple_count;
	uint64		tuple_len;
	double		tuple_percent;
	uint64		dead_tuple_count;
	uint64		dead_tuple_len;
	double		dead_tuple_percent;
	uint64		free_space;
	double		free_percent;
} output_type;
/* …（同文件无关代码省略）… */
typedef struct StatApproxReadStreamPrivate
{
	Relation	rel;
	output_type *stat;
	BlockNumber current_blocknum;
	BlockNumber nblocks;
	BlockNumber scanned;		/* count of pages actually read */
	Buffer		vmbuffer;		/* for VM lookups */
} StatApproxReadStreamPrivate;
  // <<< BUG ANCHOR
/* …（同文件无关代码省略）… */
	p.current_blocknum = 0;
	p.nblocks = nblocks;
	p.scanned = 0;
	p.vmbuffer = InvalidBuffer;

	/*
/* …（同文件无关代码省略）… */

		stat->free_space += PageGetExactFreeSpace(page);

		if (PageIsNew(page) || PageIsEmpty(page))
		{
			UnlockReleaseBuffer(buf);
			continue;
/* …（同文件无关代码省略）… */
	 * we already accounted for the space in those pages, too.
	 */
	stat->tuple_count = vac_estimate_reltuples(rel, nblocks, p.scanned,
											   stat->tuple_count);

	/* It's not clear if we could get -1 here, but be safe. */
/* …（同文件无关代码省略）… */
	 */
	if (nblocks != 0)
	{
		stat->scanned_percent = 100.0 * p.scanned / nblocks;
		stat->tuple_percent = 100.0 * stat->tuple_len / stat->table_len;
		stat->dead_tuple_percent = 100.0 * stat->dead_tuple_len / stat->table_len;
