// AUTO-DRAFT from postgres/postgres PR #c1c930376854dda25ce07a74d34d1a16f91cd3aa
		}
	}

	/*
	 * Identify any nestloop parameters that should be supplied by this join
	 * node, and remove them from root->curOuterParams.
