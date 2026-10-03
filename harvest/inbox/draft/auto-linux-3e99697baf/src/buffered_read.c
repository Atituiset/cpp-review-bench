// AUTO-DRAFT from torvalds/linux PR #ff47652a4b66c067c765a7ad464d930b5a9367cc

	ret = netfs_wait_for_read(rreq);
	if (ret >= 0) {
		if (group)
			folio_change_private(folio, group);
		else
