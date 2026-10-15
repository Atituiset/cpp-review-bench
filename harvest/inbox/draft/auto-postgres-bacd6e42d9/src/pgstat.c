// AUTO-DRAFT from postgres/postgres PR #3c6665bf0266a7ae78117c3ed0299bfbd1f1331a
			ereport(ERROR,
					(errmsg("failed to register custom cumulative statistics \"%s\" with ID %u", kind_info->name, kind),
					 errhint("Custom cumulative statistics cannot use a dedicated hash table for fixed-numbered objects.")));
	}
	else
	{
