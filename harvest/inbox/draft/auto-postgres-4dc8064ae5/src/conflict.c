// AUTO-DRAFT from postgres/postgres PR #ad36e3608c8cb6f0848737ec81e548d4d3a0af3c
		 * when applying update or delete, such an index scan may not result
		 * in a unique tuple and we still compare the complete tuple in such
		 * cases, thus such indexes are not used here.
		 */
		Oid			replica_index = GetRelationIdentityOrPK(localrel);
