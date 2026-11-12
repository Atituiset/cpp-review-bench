// AUTO-DRAFT from postgres/postgres PR #cf44f3fa19b6c35633e043621d325d55a760e472
	 */
	if (jointype == JOIN_LEFT || jointype == JOIN_FULL)
	{
		parse->mergeJoinCondition =
			add_nulling_relids(parse->mergeJoinCondition,
							   bms_make_singleton(sourcerti),  // <<< BUG ANCHOR
							   bms_make_singleton(joinrti));

		foreach_node(MergeAction, action, parse->mergeActionList)
		{
			action->qual =
				add_nulling_relids(action->qual,
								   bms_make_singleton(sourcerti),
								   bms_make_singleton(joinrti));

			action->targetList = (List *)
				add_nulling_relids((Node *) action->targetList,
								   bms_make_singleton(sourcerti),
								   bms_make_singleton(joinrti));
		}

		parse->returningList = (List *)
			add_nulling_relids((Node *) parse->returningList,
							   bms_make_singleton(sourcerti),
							   bms_make_singleton(joinrti));
	}
