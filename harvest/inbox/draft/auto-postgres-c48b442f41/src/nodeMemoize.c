// AUTO-DRAFT from postgres/postgres PR #3f67ce4819ff6199165431941463dd9d0f6e5a5a
#define MEMO_FILLING_CACHE			3	/* Read outer node to fill cache */
/* …（同文件无关代码省略）… */
					 * cache lookups to work even when the scan has not been
					 * executed to completion.
					 */
					entry->complete = node->singlerow;  // <<< BUG ANCHOR
					node->mstatus = MEMO_FILLING_CACHE;
				}
