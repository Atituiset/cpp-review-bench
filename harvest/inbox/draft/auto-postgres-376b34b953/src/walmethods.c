// AUTO-DRAFT from postgres/postgres PR #650925dad03ae3f5048e234d167c9f15df1e9853
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
  // <<< BUG ANCHOR
static char *
dir_get_file_name(WalWriteMethod *wwmethod,
				  const char *pathname, const char *temp_suffix)
{
	char	   *filename = pg_malloc0_array(char, MAXPGPATH);

	snprintf(filename, MAXPGPATH, "%s%s%s",
			 pathname,
			 wwmethod->compression_algorithm == PG_COMPRESSION_GZIP ? ".gz" :
			 wwmethod->compression_algorithm == PG_COMPRESSION_LZ4 ? ".lz4" : "",
			 temp_suffix ? temp_suffix : "");

	return filename;
}
/* …（同文件无关代码省略）… */
#endif
		r = close(df->fd);

	if (r == 0)
	{
		/* Build path to the current version of the file */
		if (method == CLOSE_NORMAL && df->temp_suffix)
/* …（同文件无关代码省略）… */
				}
			}
		}
		else if (method == CLOSE_UNLINK)
		{
			char	   *filename;

			/* Unlink the file once it's closed */
			filename = dir_get_file_name(f->wwmethod, df->base.pathname,
										 df->temp_suffix);
			snprintf(tmppath, sizeof(tmppath), "%s/%s",
					 dir_data->basedir, filename);
			pg_free(filename);
			r = unlink(tmppath);
		}
		else
		{
			/*
/* …（同文件无关代码省略）… */
	LZ4F_freeCompressionContext(df->ctx);
#endif

	pg_free(df->base.pathname);
	pg_free(df->fullpath);
	pg_free(df->temp_suffix);
