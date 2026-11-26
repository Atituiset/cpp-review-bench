// AUTO-DRAFT from postgres/postgres PR #56227fd51c452624b1c08947e4ed910a8d78748e
 *  // <<< BUG ANCHOR
 *****************************************************************************/

/*
 * pg_strtok --- retrieve next "token" from a string.
 *
/* …（同文件无关代码省略）… */

	local_str = ctx->str;

	while (*local_str == ' ' || *local_str == '\n' || *local_str == '\t')
		local_str++;

	if (*local_str == '\0')
/* …（同文件无关代码省略）… */
	 */
	ret_str = local_str;

	if (*local_str == '(' || *local_str == ')' ||
		*local_str == '{' || *local_str == '}')
	{
		/* special 1-character token */
		local_str++;
	}
	else
	{
		/* Normal token, possibly containing backslashes */
		while (*local_str != '\0' &&
			   *local_str != ' ' && *local_str != '\n' &&
			   *local_str != '\t' &&
			   *local_str != '(' && *local_str != ')' &&
			   *local_str != '{' && *local_str != '}')
		{
			if (*local_str == '\\' && local_str[1] != '\0')
				local_str += 2;
