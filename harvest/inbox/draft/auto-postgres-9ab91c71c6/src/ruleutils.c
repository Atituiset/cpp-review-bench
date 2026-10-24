// AUTO-DRAFT from postgres/postgres PR #896246ea10cb9d20f69d0442223ce5fa9b365636
  // <<< BUG ANCHOR
		/* might be returning old/new column value */
		if (var->varreturningtype == VAR_RETURNING_OLD)
		{
			refname = dpns->ret_old_alias;
			Assert(refname);
		}
		else if (var->varreturningtype == VAR_RETURNING_NEW)
		{
			refname = dpns->ret_new_alias;
			Assert(refname);
		}
		else
			refname = (char *) list_nth(dpns->rtable_names, varno - 1);
