// AUTO-DRAFT from postgres/postgres PR #857340b0263da6d6a5f764bbab2fb39d0456f9fd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define	LZ4_MAX_BLCKSZ		LZ4_COMPRESSBOUND(BLCKSZ)
/* …（同文件无关代码省略）… */
#define ZSTD_MAX_BLCKSZ		ZSTD_COMPRESSBOUND(BLCKSZ)
/* …（同文件无关代码省略）… */
#define PGLZ_MAX_BLCKSZ		PGLZ_MAX_OUTPUT(BLCKSZ)
/* …（同文件无关代码省略）… */
#define COMPRESS_BUFSIZE	Max(Max(PGLZ_MAX_BLCKSZ, LZ4_MAX_BLCKSZ), ZSTD_MAX_BLCKSZ)
/* …（同文件无关代码省略）… */
static XLogRecData hdr_rdt;
static char *hdr_scratch = NULL;

#define SizeOfXlogOrigin	(sizeof(ReplOriginId) + sizeof(char))
#define SizeOfXLogTransactionId	(sizeof(TransactionId) + sizeof(char))

/* …（同文件无关代码省略）… */

		case WAL_COMPRESSION_ZSTD:
#ifdef USE_ZSTD
			len = ZSTD_compress(dest, COMPRESS_BUFSIZE, source, orig_len,
								ZSTD_CLEVEL_DEFAULT);
			if (ZSTD_isError(len))
				len = -1;		/* failure */
#else
