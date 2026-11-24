// AUTO-DRAFT from postgres/postgres PR #9ebc9a458c38465cd9609814cfc9ca3a2f46fef5
			 */
			if (GistTupleIsInvalid(idxtuple))
				ereport(ERROR,
						(errmsg("index \"%s\" contains an inner tuple marked as invalid",  // <<< BUG ANCHOR
								RelationGetRelationName(r)),
						 errdetail("This is caused by an incomplete page split at crash recovery before upgrading to PostgreSQL 9.1."),
						 errhint("Please REINDEX it.")));
