// AUTO-DRAFT from postgres/postgres PR #6f3bdadaadc4692050ca20dff53d3890088b9272
typedef struct ColumnIOData ColumnIOData;
/* …（同文件无关代码省略）… */
	Oid			record_type;
	int32		record_typmod;
	int			ncolumns;
	ColumnIOData columns[FLEXIBLE_ARRAY_MEMBER] pg_attribute_counted_by(ncolumns);  // <<< BUG ANCHOR
};

/* per-query cache for populate_record_worker and populate_recordset_worker */
