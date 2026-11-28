// AUTO-DRAFT from postgres/postgres PR #7622c7db525377a924e7b7fce2b9c74bda7c9856
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
						   *p1,
						   *p2;

				ret = strdup(t);

				fclose(fp);
/* …（同文件无关代码省略）… */
					return NULL;
				}

				/* De-escape password. */
				for (p1 = p2 = ret; *p1 != ':' && *p1 != '\0'; ++p1, ++p2)
				{
					if (*p1 == '\\' && p1[1] != '\0')
						++p1;
					*p2 = *p1;
				}
				*p2 = '\0';

				return ret;
			}
		}
