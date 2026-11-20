// AUTO-DRAFT from postgres/postgres PR #0cb41ee8070fb0930d2ead13e19304d11d405636
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct DirectoryMethodData
{
	WalWriteMethod base;
	char	   *basedir;
} DirectoryMethodData;
/* …（同文件无关代码省略）… */
typedef struct DirectoryMethodFile
{
	Walfile		base;
	int			fd;
	char	   *fullpath;
	char	   *temp_suffix;
#ifdef HAVE_LIBZ
	gzFile		gzfp;
#endif
#ifdef USE_LZ4
	LZ4F_compressionContext_t ctx;
	size_t		lz4bufsize;
	void	   *lz4buf;
#endif
} DirectoryMethodFile;
/* …（同文件无关代码省略）… */
static int
dir_close(Walfile *f, WalCloseMethod method)
{
	int			r;
	DirectoryMethodFile *df = (DirectoryMethodFile *) f;
	DirectoryMethodData *dir_data = (DirectoryMethodData *) f->wwmethod;
	char		tmppath[MAXPGPATH];
/* …（同文件无关代码省略）… */
		if (LZ4F_isError(compressed))
		{
			f->wwmethod->lasterrstring = LZ4F_getErrorName(compressed);
			return -1;
		}

		errno = 0;
		if (write(df->fd, df->lz4buf, compressed) != compressed)
		{
			/* If write didn't set errno, assume problem is no disk space */
			f->wwmethod->lasterrno = errno ? errno : ENOSPC;
			return -1;
		}

		r = close(df->fd);
/* …（同文件无关代码省略）… */
		f->wwmethod->lasterrno = errno;

#ifdef USE_LZ4
	pg_free(df->lz4buf);
	/* supports free on NULL */
	LZ4F_freeCompressionContext(df->ctx);
