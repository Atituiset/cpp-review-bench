// AUTO-DRAFT from postgres/postgres PR #ce3387b74b708a679282ba5c72990b532d661bb8
	 * ----------------------------------------
	 */

	/*
	 * Buffer for decoded records.  This is a circular buffer, though
	 * individual records can't be split in the middle, so some space is often
