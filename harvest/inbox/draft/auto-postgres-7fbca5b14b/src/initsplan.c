// AUTO-DRAFT from postgres/postgres PR #48938780c3577835dda3dd17db4f8c968bb1e166
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	Bitmapset **groupbyattnos;
	List	  **groupbycols;
	Bitmapset **surplusvars;
	bool		tryremove = false;
	ListCell   *lc;
	int			relid;
/* …（同文件无关代码省略）… */
		relid = var->varno;
		Assert(relid <= list_length(parse->rtable));

		/*
		 * If this isn't the first column for this relation then we now have
		 * multiple columns.  That means there might be some that can be
/* …（同文件无关代码省略）… */
				 * can relax this check when the index is defined with NULLS
				 * NOT DISTINCT as there can only be 1 NULL row, therefore
				 * functional dependency on the unique columns is maintained,
				 * despite the NULL.
				 */
				if (!index->nullsnotdistinct &&
					!bms_is_member(indkey_attno, rel->notnullattnums))
				{
					index_check_ok = false;
