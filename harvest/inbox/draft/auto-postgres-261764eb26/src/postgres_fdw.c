// AUTO-DRAFT from postgres/postgres PR #b6178c942ea7a85f72beb6a775d2793062a7ff8e
	if (input_rel->reloptkind == RELOPT_BASEREL ||
		input_rel->reloptkind == RELOPT_JOINREL)
	{
		Assert(root->query_pathkeys == root->sort_pathkeys ||  // <<< BUG ANCHOR
			   root->sort_pathkeys == NIL);

		/* Safe to push down if the query_pathkeys are safe to push down */
		fpinfo->pushdown_safe = ifpinfo->qp_is_pushdown_safe;
		return;
	}
