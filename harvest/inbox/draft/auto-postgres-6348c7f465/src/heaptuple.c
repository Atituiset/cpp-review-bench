// AUTO-DRAFT from postgres/postgres PR #610b1a6efbbee043e3ad28121d61fb4a762ee42b
	/* log details about the mismatch */
	ereport(LOG_SERVER_ONLY,
			errcode(ERRCODE_DATUM_VERSION_MISMATCH),
			errmsg("tuple descriptor for type %s (OID %u), version %X/%X is incompatible with tuple type %s (OID %u), version %X/%X",  // <<< BUG ANCHOR
				   format_type_be(descTypeid), descTypeid,
				   LSN_FORMAT_ARGS(descVersion),
				   format_type_be(tupTypeid), tupTypeid,
/* …（同文件无关代码省略）… */
		 * RECORDOID, but could be reached if some caller relabels a versioned
		 * datum as RECORDOID without going through heap_tuple_set_datum().
		 */
		elog(ERROR, "processing record datum (%u, %d) with rowtype tupdesc (%u, %d, %X/%X)",
			 tupTypeid, tupTypmod,
			 tupleDesc->tdtypeid, tupleDesc->tdtypmod,
			 LSN_FORMAT_ARGS(descVersion));
