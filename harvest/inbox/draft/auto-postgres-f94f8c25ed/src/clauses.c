// AUTO-DRAFT from postgres/postgres PR #d8444163d22e576c9f284a55eab7999ba67c4ceb
				attr = TupleDescCompactAttr(RelationGetDescr(rel),
											var->varattno - 1);
				result = (attr->attnullability == ATTNULLABLE_VALID);
				table_close(rel, NoLock);

				return result;
