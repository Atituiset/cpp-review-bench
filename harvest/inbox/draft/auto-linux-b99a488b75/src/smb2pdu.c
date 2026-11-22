// AUTO-DRAFT from torvalds/linux PR #62b13f41be6284250c06ab4d79a7fb13fe93f4d7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
					      rc);
				rdata->subreq.error = rc;
				rdata->result = rc;

				if (is_replayable_error(rc)) {
					trace_netfs_sreq(&rdata->subreq, netfs_sreq_trace_io_retry_needed);
/* …（同文件无关代码省略）… */
			__set_bit(NETFS_SREQ_CLEAR_TAIL, &rdata->subreq.flags);
	} else {
		size_t trans = rdata->subreq.transferred + rdata->got_bytes;
		if (trans < rdata->subreq.len &&
		    rdata->subreq.start + trans >= netfs_read_remote_i_size(inode)) {
			rdata->result = 0;
			if (rdata->subreq.start + trans >= i_size_read(inode))
				__set_bit(NETFS_SREQ_HIT_EOF, &rdata->subreq.flags);
			else
/* …（同文件无关代码省略）… */
		if (written > wdata->subreq.len)
			written &= 0xFFFF;

		cifs_stats_bytes_written(tcon, written);

		if (written < wdata->subreq.len) {
			result = -ENOSPC;
		} else if (written > 0) {
			wdata->subreq.len = written;
			__set_bit(NETFS_SREQ_MADE_PROGRESS, &wdata->subreq.flags);
		}
		break;
	case MID_REQUEST_SUBMITTED:
		trace_netfs_sreq(&wdata->subreq, netfs_sreq_trace_io_req_submitted);
