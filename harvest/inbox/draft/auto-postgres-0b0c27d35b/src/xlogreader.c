// AUTO-DRAFT from postgres/postgres PR #01b3d24c41629c577ca172f3077ec7bfc2ca9bf3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define MAX_ERRORMSG_LEN 1000
/* …（同文件无关代码省略）… */
static void
report_invalid_record(XLogReaderState *state, const char *fmt, ...)
{
	va_list		args;

	fmt = _(fmt);

	va_start(args, fmt);
	vsnprintf(state->errormsg_buf, MAX_ERRORMSG_LEN, fmt, args);
	va_end(args);

	state->errormsg_deferred = true;
}
/* …（同文件无关代码省略）… */
				report_invalid_record(state,
									  "out-of-order block_id %u at %X/%08X",
									  block_id,
									  LSN_FORMAT_ARGS(state->ReadRecPtr));
				goto err;
			}
			decoded->max_block_id = block_id;
/* …（同文件无关代码省略）… */
			{
				report_invalid_record(state,
									  "BKPBLOCK_HAS_DATA set, but no data included at %X/%08X",
									  LSN_FORMAT_ARGS(state->ReadRecPtr));
				goto err;
			}
			if (!blk->has_data && blk->data_len != 0)
			{
				report_invalid_record(state,
									  "BKPBLOCK_HAS_DATA not set, but data length is %d at %X/%08X",
									  blk->data_len,
									  LSN_FORMAT_ARGS(state->ReadRecPtr));
				goto err;
			}
			datatotal += blk->data_len;
/* …（同文件无关代码省略）… */
										  blk->hole_offset,
										  blk->hole_length,
										  blk->bimg_len,
										  LSN_FORMAT_ARGS(state->ReadRecPtr));
					goto err;
				}

/* …（同文件无关代码省略）… */
										  "BKPIMAGE_HAS_HOLE not set, but hole offset %d length %d at %X/%08X",
										  blk->hole_offset,
										  blk->hole_length,
										  LSN_FORMAT_ARGS(state->ReadRecPtr));
					goto err;
				}

/* …（同文件无关代码省略）… */
					report_invalid_record(state,
										  "BKPIMAGE_COMPRESSED set, but block image length %d at %X/%08X",
										  blk->bimg_len,
										  LSN_FORMAT_ARGS(state->ReadRecPtr));
					goto err;
				}

/* …（同文件无关代码省略）… */
					report_invalid_record(state,
										  "neither BKPIMAGE_HAS_HOLE nor BKPIMAGE_COMPRESSED set, but block image length is %d at %X/%08X",
										  blk->bimg_len,
										  LSN_FORMAT_ARGS(state->ReadRecPtr));
					goto err;
				}
			}
/* …（同文件无关代码省略）… */
				{
					report_invalid_record(state,
										  "BKPBLOCK_SAME_REL set but no previous rel at %X/%08X",
										  LSN_FORMAT_ARGS(state->ReadRecPtr));
					goto err;
				}

/* …（同文件无关代码省略）… */
		{
			report_invalid_record(state,
								  "invalid block_id %u at %X/%08X",
								  block_id, LSN_FORMAT_ARGS(state->ReadRecPtr));
			goto err;
		}
	}
/* …（同文件无关代码省略）… */
shortdata_err:
	report_invalid_record(state,
						  "record with invalid length at %X/%08X",
						  LSN_FORMAT_ARGS(state->ReadRecPtr));
err:
	*errormsg = state->errormsg_buf;
