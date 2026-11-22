// AUTO-DRAFT from torvalds/linux PR #f802b6558eb81bd5888d761a0cdc8afe5fd8c941
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
  // <<< BUG ANCHOR
 * mapping_seek_hole_data() will return it, and we will skip the punch because
 * the folio is dirty. THis is incorrect - we always need to punch out delalloc
 * beyond EOF in this case as writeback will never write back and covert that
 * delalloc block beyond EOF. Hence we limit the cached data scan range to EOF,
 * resulting in always punching out the range from the EOF to the end of the
 * range the iomap spans.
 *
 * Intervals are of the form [start_byte, end_byte) (i.e. open ended) because it
 * matches the intervals returned by mapping_seek_hole_data(). i.e. SEEK_DATA
/* …（同文件无关代码省略）… */
		iomap_punch_t punch)
{
	loff_t punch_start_byte = start_byte;
	loff_t scan_end_byte = min(i_size_read(inode), end_byte);

	/*
	 * The caller must hold invalidate_lock to avoid races with page faults
