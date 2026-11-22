// AUTO-DRAFT from postgres/postgres PR #892cfb4cbfe0e37cc492f0e750e80cfa6394d26d
			 */
			if (PQnfields(res) != 2 || PQntuples(res) != 1)
			{
				pg_log_warning("unexpected response to TIMELINE_HISTORY command: got %d rows and %d fields, expected %d rows and %d fields",  // <<< BUG ANCHOR
							   PQntuples(res), PQnfields(res), 1, 2);
			}

			/* Write the history file to disk */
