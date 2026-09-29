// AUTO-DRAFT from postgres/postgres PR #6dc3fd90f126fa409eac51a4182ee09b9d63c16c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
		if (!concurrent)
			LockRelationOid(OldHeap->rd_rel->reltoastrelid, lmode);
		else
			CheckRelationOidLockedByMe(OldHeap->rd_rel->reltoastrelid,
									   lmode, false);
	}

	/*
