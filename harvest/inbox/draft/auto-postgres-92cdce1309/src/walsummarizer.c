// AUTO-DRAFT from postgres/postgres PR #cb1eef81c9bc8d1a2adc1e3a63669e315b6232bd
		state->seg.ws_file = BasicOpenFile(path, O_RDONLY | PG_BINARY);
		if (state->seg.ws_file >= 0)
		{
			*tli_p = tli;
			return;
		}
