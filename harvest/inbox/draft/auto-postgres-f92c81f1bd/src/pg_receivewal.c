// AUTO-DRAFT from postgres/postgres PR #e87b024bfb8dd55e3049b1b6e6ab4e810cf1abc8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>

			int			fd;
			ssize_t		r;
			size_t		uncompressed_size = 0;
			char		fullpath[MAXPGPATH * 2];
			char	   *outbuf;
			char	   *readbuf;
/* …（同文件无关代码省略）… */

					readp += read_size;
					uncompressed_size += out_size;
				}

				/*
/* …（同文件无关代码省略）… */
							   dirent->d_name, uncompressed_size);
				continue;
			}
#else
			pg_log_error("cannot check file \"%s\": compression with %s not supported by this build",
						 dirent->d_name, "LZ4");
