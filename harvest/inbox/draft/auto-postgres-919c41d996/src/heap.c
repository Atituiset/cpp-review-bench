// AUTO-DRAFT from postgres/postgres PR #700b422af7e26cf310caf3fb76d78f4c8ca39c1f

	Assert(attnum > InvalidAttrNumber);

	constrOid =
		CreateConstraintEntry(nnname,
							  RelationGetNamespace(rel),
