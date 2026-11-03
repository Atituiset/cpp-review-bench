// AUTO-DRAFT from postgres/postgres PR #30faf13889b14e1bdfd0b5883291f3297135ce02
  // <<< BUG ANCHOR
	if (errormsg)
		pg_fatal("error in WAL record at %X/%08X: %s",
				 LSN_FORMAT_ARGS(xlogreader_state->ReadRecPtr),
				 errormsg);

	/*
