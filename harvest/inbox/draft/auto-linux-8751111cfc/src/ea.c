// AUTO-DRAFT from torvalds/linux PR #12419f306c3ca34a38064d2b502966fa01650b49
		goto err_out;
	}

	if (!a->non_resident) {
		if (!(new_aflags & (ATTR_IS_SPARSE | ATTR_IS_COMPRESSED))) {
			err = 0;
