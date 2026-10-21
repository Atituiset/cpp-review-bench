// AUTO-DRAFT from postgres/postgres PR #3745790f53bc5dd87b2df3d5a0c9c62d7ff0360d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define WAITOPERATOR	3
/* …（同文件无关代码省略）… */
				else
				{
					long		lval;

					nnn[innn] = '\0';
					errno = 0;
					lval = strtol(nnn, NULL, 0);
					*val = (int32) lval;
					if (errno != 0 || (long) *val != lval)
						return ERR;
					state->state = WAITOPERATOR;
					return (state->count && *(state->buf) == '\0')
