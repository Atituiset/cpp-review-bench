// AUTO-DRAFT from postgres/postgres PR #cab16d214bc5b70dd3514c5631ffa1f0acb3fed3
			}

			/*
			 * Otherwise, if the specified targets exactly match this
			 * joinrel, arrange to enforce it here; see comments below for
			 * details on what we do when there is not an exact match.
			 */
			if (itm == PGPA_ITM_EQUAL)
			{
