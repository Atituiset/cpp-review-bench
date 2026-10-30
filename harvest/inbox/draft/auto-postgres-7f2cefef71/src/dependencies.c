// AUTO-DRAFT from postgres/postgres PR #b0392c26bd3cfb61ee24b7577967cc806d01da36
		/* make sure the attnum is positive (valid AttrNumber) */
		attnum = list_attnums[i] + attnum_offset;
  // <<< BUG ANCHOR
		/*
		 * Either it's a regular attribute, or it's an expression, in which
		 * case we must not have seen it before (expressions are unique).
		 *
		 * XXX Check whether it's a regular attribute has to be done using the
		 * original attnum, while the second check has to use the value with
		 * an offset.
		 */
		Assert(AttrNumberIsForUserDefinedAttr(list_attnums[i]) ||
			   !bms_is_member(attnum, clauses_attnums));

		/*
		 * Remember the offset attnum, both for attributes and expressions.
		 * We'll pass list_attnums to clauselist_apply_dependencies, which
