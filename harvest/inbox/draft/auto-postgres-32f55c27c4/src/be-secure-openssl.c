// AUTO-DRAFT from postgres/postgres PR #5c74e122ef295e251be43a8ce0c561e91fcd000d
	{
		ereport(WARNING,
				errcode(ERRCODE_CONFIG_FILE_ERROR),
				errmsg("SSL configuration not reloaded, SNI remains %s", SSL_hosts->sni_enabled ? "on" : "off"),  // <<< BUG ANCHOR
				errdetail("The SSL configuration failed to reload, previous configuration and SNI state will remain active."));
	}

	MemoryContextSwitchTo(oldcxt);
