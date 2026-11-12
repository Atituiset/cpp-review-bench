// AUTO-DRAFT from postgres/postgres PR #8d87367b39e44b0913ca585c733795cc2d30bff1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
 * Do parallel index bulk-deletion with parallel workers.
 */
void
parallel_vacuum_bulkdel_all_indexes(ParallelVacuumState *pvs, long num_table_tuples,
									int num_index_scans, PVWorkerStats *wstats)
{
	Assert(!IsParallelWorker());
/* …（同文件无关代码省略）… */
 * Do parallel index cleanup with parallel workers.
 */
void
parallel_vacuum_cleanup_all_indexes(ParallelVacuumState *pvs, long num_table_tuples,
									int num_index_scans, bool estimated_count,
									PVWorkerStats *wstats)
{
