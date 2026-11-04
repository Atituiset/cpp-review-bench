// AUTO-DRAFT from torvalds/linux PR #508d578642f08a3ffde7631c454497bd5a735941
			continue;
		}

		/* unsync and write-flooding only apply to indirect SPs. */
		if (sp->role.direct)
			goto out;
