// AUTO-DRAFT from postgres/postgres PR #9b80b3170b9c5be2f22540facea52f5e38915704
		 */
		child->initPlan = list_concat(parent->initPlan,
									  child->initPlan);
	}

	/*
