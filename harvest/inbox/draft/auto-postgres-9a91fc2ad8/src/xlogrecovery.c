// AUTO-DRAFT from postgres/postgres PR #27caf4ca5ae5ecfe66822d5a9c5803e0d1bc2ed0
			continue;
		}
  // <<< BUG ANCHOR
		Assert(XLogRecHasBlockImage(record, block_id));

		if (XLogRecBlockImageApply(record, block_id))
		{
