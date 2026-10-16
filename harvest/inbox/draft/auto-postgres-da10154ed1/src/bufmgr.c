// AUTO-DRAFT from postgres/postgres PR #eb7cc1d09e742d369540016455d0f55c619106f2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

static inline void
buffer_readv_decode_error(PgAioResult result,
						  bool *zeroed_any,
						  bool *ignored_any,
						  uint8 *zeroed_or_error_count,
						  uint8 *checkfail_count,
						  uint8 *first_off)
{
	uint32		rem_error = result.error_data;

	/* see static asserts in buffer_readv_encode_error */
#define READV_COUNT_BITS	7
#define READV_COUNT_MASK	((1 << READV_COUNT_BITS) - 1)

	*zeroed_any = rem_error & 1;
	rem_error >>= 1;

	*ignored_any = rem_error & 1;
	rem_error >>= 1;

	*zeroed_or_error_count = rem_error & READV_COUNT_MASK;
	rem_error >>= READV_COUNT_BITS;

	*checkfail_count = rem_error & READV_COUNT_MASK;
	rem_error >>= READV_COUNT_BITS;

	*first_off = rem_error & READV_COUNT_MASK;
	rem_error >>= READV_COUNT_BITS;
}
/* …（同文件无关代码省略）… */
static inline void
buffer_readv_encode_error(PgAioResult *result,
						  bool is_temp,
						  bool zeroed_any,
						  bool ignored_any,
						  uint8 error_count,
						  uint8 zeroed_count,
						  uint8 checkfail_count,
						  uint8 first_error_off,
						  uint8 first_zeroed_off,
						  uint8 first_ignored_off)
{

	uint8		shift = 0;
	uint8		zeroed_or_error_count =
		error_count > 0 ? error_count : zeroed_count;
	uint8		first_off;

	StaticAssertDecl(PG_IOV_MAX <= 1 << READV_COUNT_BITS,
					 "PG_IOV_MAX is bigger than reserved space for error data");
	StaticAssertDecl((1 + 1 + 3 * READV_COUNT_BITS) <= PGAIO_RESULT_ERROR_BITS,
					 "PGAIO_RESULT_ERROR_BITS is insufficient for buffer_readv");

	/*
	 * We only have space to encode one offset - but luckily that's good
	 * enough. If there is an error, the error is the interesting offset, same
	 * with a zeroed buffer vs an ignored buffer.
	 */
	if (error_count > 0)
		first_off = first_error_off;
	else if (zeroed_count > 0)
		first_off = first_zeroed_off;
	else
		first_off = first_ignored_off;

	Assert(!zeroed_any || error_count == 0);

	result->error_data = 0;

	result->error_data |= zeroed_any << shift;
	shift += 1;

	result->error_data |= ignored_any << shift;
	shift += 1;

	result->error_data |= ((uint32) zeroed_or_error_count) << shift;
	shift += READV_COUNT_BITS;

	result->error_data |= ((uint32) checkfail_count) << shift;
	shift += READV_COUNT_BITS;

	result->error_data |= ((uint32) first_off) << shift;
	shift += READV_COUNT_BITS;

	result->id = is_temp ? PGAIO_HCB_LOCAL_BUFFER_READV :
		PGAIO_HCB_SHARED_BUFFER_READV;

	if (error_count > 0)
		result->status = PGAIO_RS_ERROR;
	else
		result->status = PGAIO_RS_WARNING;

	/*
	 * The encoding is complicated enough to warrant cross-checking it against
	 * the decode function.
	 */
#ifdef USE_ASSERT_CHECKING
	{
		bool		zeroed_any_2,
					ignored_any_2;
		uint8		zeroed_or_error_count_2,
					checkfail_count_2,
					first_off_2;

		buffer_readv_decode_error(*result,
								  &zeroed_any_2, &ignored_any_2,
								  &zeroed_or_error_count_2,
								  &checkfail_count_2,
								  &first_off_2);
		Assert(zeroed_any == zeroed_any_2);
		Assert(ignored_any == ignored_any_2);
		Assert(zeroed_or_error_count == zeroed_or_error_count_2);
		Assert(checkfail_count == checkfail_count_2);
		Assert(first_off == first_off_2);
	}
#endif

#undef READV_COUNT_BITS
#undef READV_COUNT_MASK
}
/* …（同文件无关代码省略）… */
			buffer_readv_encode_error(&result_one, is_temp,
									  *zeroed_buffer,
									  *ignored_checksum,
									  *buffer_invalid,
									  *zeroed_buffer ? 1 : 0,
									  *failed_checksum ? 1 : 0,
									  buf_off, buf_off, buf_off);
