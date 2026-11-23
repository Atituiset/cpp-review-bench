// AUTO-DRAFT from postgres/postgres PR #610b1a6efbbee043e3ad28121d61fb4a762ee42b
		ereport(elevel,
				errmsg("attribute versions of relation \"%s\" are non-monotonic",
					   relname),
				errdetail("Version %X/%X of attribute \"%s\" at position %d is lower than version %X/%X of attribute \"%s\" at position %d.",  // <<< BUG ANCHOR
						  LSN_FORMAT_ARGS(attVersion),
						  NameStr(TupleDescAttr(td, attnum - 1)->attname),
						  attnum,
