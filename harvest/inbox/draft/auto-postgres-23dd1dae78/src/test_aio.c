// AUTO-DRAFT from postgres/postgres PR #9872a0de75b1d4b178681008543a0e2dffd5fda4
  // <<< BUG ANCHOR
	pgaio_io_set_handle_data_32(ioh, (uint32 *) bufs, nblocks);

	if (zero_on_error | zero_damaged_pages)
		srb_flags |= READ_BUFFERS_ZERO_ON_ERROR;
	if (ignore_checksum_failure)
		srb_flags |= READ_BUFFERS_IGNORE_CHECKSUM_FAILURES;
