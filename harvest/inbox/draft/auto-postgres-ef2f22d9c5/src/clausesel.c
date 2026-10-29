// AUTO-DRAFT from postgres/postgres PR #f400289946f7d0e66a4169cf4aa56079f1422a2b
				return (Selectivity) 1.0;
		}

		/*
		 * If possible, cache the result of the selectivity calculation for
		 * the clause.  We can cache if varRelid is zero or the clause
