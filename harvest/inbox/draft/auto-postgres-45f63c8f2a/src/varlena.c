// AUTO-DRAFT from postgres/postgres PR #978772cf5dd642b093382e9ff4b3a353e8a5d1a2
	if (n < 0)
	{
		/*
		 * Negating PG_INT32_MIN would overflow, so clamp instead.  Any n whose
		 * absolute value is at least the string's length skips the whole
		 * string, and len can't exceed PG_INT32_MAX, so this is equivalent.
		 */
		if (unlikely(n == PG_INT32_MIN))
			n = PG_INT32_MAX;
