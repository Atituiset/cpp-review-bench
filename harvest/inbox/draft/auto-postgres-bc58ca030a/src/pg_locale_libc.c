// AUTO-DRAFT from postgres/postgres PR #8474bae2ba4cf8c6e91db800a441daffe22098ee
  // <<< BUG ANCHOR
	/*
	 * For UTF8, pg_wchar is a codepoint and we assume we can pass it directly
	 * to iswupper_l()/iswlower_l().
	 */
	return iswupper_l((wint_t) wc, locale->lt) ||
		iswlower_l((wint_t) wc, locale->lt);
}

static pg_wchar
