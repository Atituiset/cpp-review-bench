// AUTO-DRAFT from postgres/postgres PR #11b56ee27315276bd09eff17fa503ba39f4231ce
	{
		Var		   *var = (Var *) lfirst(lc);
  // <<< BUG ANCHOR
		/* Ignore placeholders */
		if (!IsA(var, Var))
			continue;
		Assert(var->varno == rel->relid);

		/* If we find a Var referencing this TLE, we're good */
		if (var->varattno == tle->resno)
