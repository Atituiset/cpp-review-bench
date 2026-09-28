// AUTO-DRAFT from postgres/postgres PR #6a2edf2a1e6d0c277b7ddf9604fe5b83766cc8e1
	if (len > 0 && message[len - 1] == '\n')
		len--;
  // <<< BUG ANCHOR
	ereport(LOG,
			errmsg_internal("%s: %.*s", prefix, len, message));
}

#define PGresult libpqsrv_PGresult
