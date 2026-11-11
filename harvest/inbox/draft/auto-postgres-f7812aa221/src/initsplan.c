// AUTO-DRAFT from postgres/postgres PR #8cdf3c9214feb48179a3cc0fbb0bae0e0f38f2cb
 *	  If the restrictinfo's clause is suitable to be used for a Memoize node,
 *	  set the left_hasheqoperator and right_hasheqoperator to the hash equality
 *	  operator that will be needed during caching.
 */
static void
check_memoizable(RestrictInfo *restrictinfo)
{
	TypeCacheEntry *typentry;
	Expr	   *clause = restrictinfo->clause;
	Oid			lefttype;
	Oid			righttype;
  // <<< BUG ANCHOR
/* …（同文件无关代码省略）… */
		return;

	lefttype = exprType(linitial(((OpExpr *) clause)->args));

	typentry = lookup_type_cache(lefttype, TYPECACHE_HASH_PROC |
								 TYPECACHE_EQ_OPR);

	if (OidIsValid(typentry->hash_proc) && OidIsValid(typentry->eq_opr))
		restrictinfo->left_hasheqoperator = typentry->eq_opr;

	righttype = exprType(lsecond(((OpExpr *) clause)->args));

	/*
	 * Lookup the right type, unless it's the same as the left type, in which
	 * case typentry is already pointing to the required TypeCacheEntry.
	 */
	if (lefttype != righttype)
		typentry = lookup_type_cache(righttype, TYPECACHE_HASH_PROC |
									 TYPECACHE_EQ_OPR);

	if (OidIsValid(typentry->hash_proc) && OidIsValid(typentry->eq_opr))
		restrictinfo->right_hasheqoperator = typentry->eq_opr;
}
