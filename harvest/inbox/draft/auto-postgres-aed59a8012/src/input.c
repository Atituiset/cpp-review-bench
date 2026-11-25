// AUTO-DRAFT from postgres/postgres PR #5ff67951c46d304d5ac4cda62eb99d64cd292c8c
#define NL_IN_HISTORY	0x01
/* …（同文件无关代码省略）… */
		char	   *cur_ptr;
  // <<< BUG ANCHOR
		/* some platforms declare HIST_ENTRY.line as const char * */
		for (cur_ptr = (char *) cur_hist->line; *cur_ptr; cur_ptr++)
		{
			if (*cur_ptr == '\n')
				*cur_ptr = NL_IN_HISTORY;
/* …（同文件无关代码省略）… */
		char	   *cur_ptr;

		/* some platforms declare HIST_ENTRY.line as const char * */
		for (cur_ptr = (char *) cur_hist->line; *cur_ptr; cur_ptr++)
		{
			if (*cur_ptr == NL_IN_HISTORY)
				*cur_ptr = '\n';
