// AUTO-DRAFT from postgres/postgres PR #282111b45c86b9e61809dbf3bd4a74ec0f6a4807
typedef struct WaitStmt
{
	NodeTag		type;
	char	   *lsn_literal;	/* LSN string from grammar */  // <<< BUG ANCHOR
	List	   *options;		/* List of DefElem nodes */
} WaitStmt;

