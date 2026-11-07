// AUTO-DRAFT from postgres/postgres PR #4a934833fb02ac2a18d0a3f86b67b493d7fe8979
	 * checkpoint the same way a restartpoint horizon can.
	 *
	 * Never adopt over a state over the watermark or a node-local
	 * pg_checksums change, it generats no WAL so nothing in the replayed WAL
	 * could restore it once overwritten.  Otherwise, even a watermark above
	 * the redo point must not prevent adoption.  A primary backup copies
	 * pg_control after the relation files, which may still contain pages
