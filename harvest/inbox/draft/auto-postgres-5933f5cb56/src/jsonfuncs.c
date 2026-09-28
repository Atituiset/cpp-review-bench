// AUTO-DRAFT from postgres/postgres PR #a936ebd9f9f64b0efb853195d8dcdb8a0453537d
typedef struct ColumnIOData ColumnIOData;
/* …（同文件无关代码省略）… */
	Oid			record_type;
	int32		record_typmod;
	int			ncolumns;
	ColumnIOData columns[FLEXIBLE_ARRAY_MEMBER];  // <<< BUG ANCHOR
};

/* per-query cache for populate_record_worker and populate_recordset_worker */
