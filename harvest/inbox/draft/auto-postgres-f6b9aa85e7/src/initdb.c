// AUTO-DRAFT from postgres/postgres PR #d870be195b3b7ee224a58ff562c1f1c252a96814
		{
			/*
			 * We try to preserve original indentation, which is tedious.
			 * oldindent and newindent are measured in de-tab-ified columns.
			 */
			const char *ptr;
			int			oldindent = 0;
			int			newindent;
  // <<< BUG ANCHOR
			for (ptr = lines[i]; ptr < where; ptr++)
			{
				if (*ptr == '\t')
					oldindent += 8 - (oldindent % 8);
				else
					oldindent++;
			}
			/* ignore the possibility of tabs in guc_value */
			newindent = newline->len;
			/* append appropriate tabs and spaces, forcing at least one */
			oldindent = Max(oldindent, newindent + 1);
			while (newindent < oldindent)
			{
				int			newindent_if_tab = newindent + 8 - (newindent % 8);

				if (newindent_if_tab <= oldindent)
				{
					appendPQExpBufferChar(newline, '\t');
					newindent = newindent_if_tab;
				}
				else
				{
					appendPQExpBufferChar(newline, ' ');
					newindent++;
				}
			}
			/* and finally append the old comment */
			appendPQExpBufferStr(newline, where);
