// AUTO-DRAFT from postgres/postgres PR #ba51d62be465c420b0fbe3b0efd0ccd488e79922
{
	ParallelBlockTableScanDesc bpscan = (ParallelBlockTableScanDesc) pscan;

	pg_atomic_write_u64(&bpscan->phs_nallocated, 0);
}
