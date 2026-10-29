// AUTO-DRAFT from postgres/postgres PR #b6e41c16f8ccbfca0d51c94bdb6043b6e8393d6f
{
	ErrorSaveContext *escontext = op->d.jsonexpr_coercion.escontext;

	/*
	 * Prepare to call json_populate_type() to coerce the boolean result of
	 * JSON_EXISTS_OP to the target type.  If the target type is integer or a
