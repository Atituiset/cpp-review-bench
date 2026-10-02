// AUTO-DRAFT from postgres/postgres PR #42e96cf2fe095c822f76bc94669ea875cb1e3351
		 */
		CHECK_FOR_INTERRUPTS();

		for (i = 0; i < pcxt->nworkers_launched; ++i)
		{
			/*
