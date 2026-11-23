// AUTO-DRAFT from postgres/postgres PR #610b1a6efbbee043e3ad28121d61fb4a762ee42b
  // <<< BUG ANCHOR
	if (res->status != WALRCV_OK_TUPLES)
		ereport(ERROR,
				errmsg("could not fetch primary slot name \"%s\" info from the primary server: %s",
					   PrimarySlotName, res->err),
				errhint("Check if \"primary_slot_name\" is configured correctly."));

/* …（同文件无关代码省略）… */
		Assert(!is_slotsync_worker);
		ereport(ERROR,
				errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("replication slot synchronization will stop because of a parameter change"));
	}

}
