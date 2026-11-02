// AUTO-DRAFT from postgres/postgres PR #633dee1bd1b41145f81619423d573754c7ad2b9e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
#define BITMAPSET_SIZE(nwords)	\
	(offsetof(Bitmapset, words) + (nwords) * sizeof(bitmapword))
/* …（同文件无关代码省略）… */
	if (a->nwords < b->nwords)
		a = (Bitmapset *) repalloc(a, BITMAPSET_SIZE(b->nwords));

	i = 0;
	do
	{
		a->words[i] = b->words[i];
	} while (++i < b->nwords);

	a->nwords = b->nwords;

#ifdef REALLOCATE_BITMAPSETS

	/*
