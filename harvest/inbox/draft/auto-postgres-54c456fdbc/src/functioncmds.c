// AUTO-DRAFT from postgres/postgres PR #8e27c9261c9dd6127e16ced998d37ed7a8cb2b7d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
			isinput = true;
			if (parameterTypes_list)
				*parameterTypes_list = lappend_oid(*parameterTypes_list, toid);
		}

		/* handle output parameters */
/* …（同文件无关代码省略）… */
			have_names = true;
		}

		if (inParameterNames_list)
			*inParameterNames_list = lappend(*inParameterNames_list, makeString(fp->name ? fp->name : pstrdup("")));

		if (fp->defexpr)
		{
			Node	   *def;
/* …（同文件无关代码省略）… */

		pinfo->fname = funcname;
		pinfo->nargs = list_length(parameterTypes);
		pinfo->argtypes = palloc_array(Oid, pinfo->nargs);
		pinfo->argnames = palloc_array(char *, pinfo->nargs);
		for (int i = 0; i < list_length(parameterTypes); i++)
		{
			char	   *s = strVal(list_nth(inParameterNames, i));
