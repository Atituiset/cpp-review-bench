// AUTO-DRAFT from postgres/postgres PR #b3dbde59376f66e063ceacc03b6d2102825de692
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
typedef struct HASHELEMENT
{
	struct HASHELEMENT *link;	/* link to next entry in same bucket */
	uint32		hashvalue;		/* hash function result for this entry */
} HASHELEMENT;
/* …（同文件无关代码省略）… */
typedef struct HTAB HTAB;
/* …（同文件无关代码省略）… */
typedef struct
{
	HTAB	   *hashp;
	uint32		curBucket;		/* index of current bucket */
	HASHELEMENT *curEntry;		/* current entry in bucket */
	bool		hasHashvalue;	/* true if hashvalue was provided */
	uint32		hashvalue;		/* hashvalue to start seqscan over hash */
} HASH_SEQ_STATUS;
/* …（同文件无关代码省略）… */
										  uint32 hashvalue);
extern void *hash_seq_search(HASH_SEQ_STATUS *status);
extern void hash_seq_term(HASH_SEQ_STATUS *status);
extern void hash_freeze(HTAB *hashp);
extern Size hash_estimate_size(int64 num_entries, Size entrysize);
extern void AtEOXact_HashTables(bool isCommit);
extern void AtEOSubXact_HashTables(bool isCommit, int nestDepth);
