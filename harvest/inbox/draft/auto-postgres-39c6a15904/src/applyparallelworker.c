// AUTO-DRAFT from postgres/postgres PR #952bab8b74d40fcfd25e1949865c3907e676504e
	switch (msgtype)
	{
		case PqMsg_ErrorResponse:
			{
				ErrorData	edata;  // <<< BUG ANCHOR

				/* Parse ErrorResponse. */
				pq_parse_errornotice(msg, &edata);

				/*
				 * If desired, add a context line to show that this is a
				 * message propagated from a parallel apply worker. Otherwise,
				 * it can sometimes be confusing to understand what actually
				 * happened.
				 */
				if (edata.context)
					edata.context = psprintf("%s\n%s", edata.context,
											 _("logical replication parallel apply worker"));
				else
					edata.context = pstrdup(_("logical replication parallel apply worker"));

				/*
				 * Context beyond that should use the error context callbacks
				 * that were in effect in LogicalRepApplyLoop().
				 */
				error_context_stack = apply_error_context_stack;

				/*
				 * The actual error must have been reported by the parallel
				 * apply worker.
				 */
				ereport(ERROR,
						(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						 errmsg("logical replication parallel apply worker exited due to error"),
						 errcontext("%s", edata.context)));
			}

			/*
			 * Don't need to do anything about NoticeResponse and
