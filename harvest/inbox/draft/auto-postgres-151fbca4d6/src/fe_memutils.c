// AUTO-DRAFT from postgres/postgres PR #38c3d87d43397ab600f085e398789f3dea1cd0a5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
pnstrdup(const char *in, Size size)
{
	char	   *tmp;
	int			len;

	if (!in)
	{
/* …（同文件无关代码省略）… */
		exit(EXIT_FAILURE);
	}

	len = strnlen(in, size);
	tmp = malloc(len + 1);
	if (tmp == NULL)
	{
		fprintf(stderr, _("out of memory\n"));
		exit(EXIT_FAILURE);
	}

	memcpy(tmp, in, len);
	tmp[len] = '\0';

	return tmp;
}
