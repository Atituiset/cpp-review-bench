// AUTO-DRAFT from postgres/postgres PR #66042bd3ebd4a7d5b614bf7435f29a8cac06fea9
pg_indexam_progress_phasename(PG_FUNCTION_ARGS)
{
	Oid			amoid = PG_GETARG_OID(0);
	int32		phasenum = PG_GETARG_INT32(1);  // <<< BUG ANCHOR
	const IndexAmRoutine *routine;
	char	   *name;
