// AUTO-DRAFT from postgres/postgres PR #fe5a0109770465b59408638dd54ea29af8f19a49
#define STACKDEPTH		32
/* …（同文件无关代码省略）… */
				}
				else
				{
					if (lenstack == STACKDEPTH)  // <<< BUG ANCHOR
						/* internal error */
						elog(ERROR, "stack too short");
					stack[lenstack] = val;
					lenstack++;
				}
/* …（同文件无关代码省略）… */
				ereturn(state->escontext, ERR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("syntax error")));

		}
	}
	while (lenstack)
