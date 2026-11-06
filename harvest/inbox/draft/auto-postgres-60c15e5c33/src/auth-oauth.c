// AUTO-DRAFT from postgres/postgres PR #8cb6b83e12043c1c91640716fb67d2ad0b988e95
		goto done;
	}

	if (!hbaline->oauth_validator)
	{
		if (elemlist->length == 1)
