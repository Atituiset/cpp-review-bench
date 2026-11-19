// AUTO-DRAFT from postgres/postgres PR #11910736bf19c806ee55ff5d5d801dbb60bbcdce
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
		dblocprovider = src_locprovider;
	if (dblocale == NULL && dblocprovider == src_locprovider)
		dblocale = src_locale;
	if (dbicurules == NULL)
		dbicurules = src_icurules;

	/* Some encodings are client only */
