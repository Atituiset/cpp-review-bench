// AUTO-DRAFT from postgres/postgres PR #456ee18045d03f8eb7194f7b7fcf53595704b44b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
 * compute an expression in this way, setrefs.c is going to insist on exact
 * matches of Vars to the source tlist.
 *
 * Child EC members are ignored unless they belong to given 'relids'.
 * Also, non-parallel-safe expressions are ignored if 'require_parallel_safe'.
 *
 * Note: some callers pass root == NULL for notational reasons.  This is OK
/* …（同文件无关代码省略）… */
			continue;

		/*
		 * Ignore child members unless they belong to the requested rel.
		 */
		if (em->em_is_child &&
			!bms_is_subset(em->em_relids, relids))
			continue;
