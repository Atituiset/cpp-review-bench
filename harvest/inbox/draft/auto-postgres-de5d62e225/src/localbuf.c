// AUTO-DRAFT from postgres/postgres PR #4b97fd15fb497d2810a29cd30bfacd89400b596c
		else if (--trycounter == 0)
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
					 errmsg("no empty local buffer available")));  // <<< BUG ANCHOR
	}

	/*
