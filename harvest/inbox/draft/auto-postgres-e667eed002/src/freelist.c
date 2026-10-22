// AUTO-DRAFT from postgres/postgres PR #4b97fd15fb497d2810a29cd30bfacd89400b596c
					 * one eventually, but it's probably better to fail than
					 * to risk getting stuck in an infinite loop.
					 */
					elog(ERROR, "no unpinned buffers available");  // <<< BUG ANCHOR
				}
				break;
			}
