// AUTO-DRAFT from postgres/postgres PR #e405a46c45bae070364ed143fb9d5a631422e5e4
	jfe->location = jt->location;
	tf->docexpr = transformExpr(pstate, (Node *) jfe, EXPR_KIND_FROM_FUNCTION);

	/*
	 * Create a JsonTablePlan that will generate row pattern that becomes
	 * source data for JSON path expressions in jt->columns.  This also adds
