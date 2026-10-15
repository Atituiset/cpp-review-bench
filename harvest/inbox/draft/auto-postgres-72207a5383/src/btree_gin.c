// AUTO-DRAFT from postgres/postgres PR #aadce2c10ffec9326a5119a3c590b77ab58b4f97

	result = timestamptz2timestamp_safe(val, (Node *) &escontext);
	/* We can ignore errors, since result is useful as-is */
	return TimestampGetDatum(result);
}
