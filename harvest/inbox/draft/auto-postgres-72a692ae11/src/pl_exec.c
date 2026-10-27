// AUTO-DRAFT from postgres/postgres PR #1dc45adefa076e9377718ee9529b375a70108734
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

	estate.err_text = gettext_noop("during initialization of execution state");
	copy_plpgsql_datums(&estate, func);

	/*
	 * Let the instrumentation plugin peek at this function
	 */
/* …（同文件无关代码省略）… */
static void
copy_plpgsql_datums(PLpgSQL_execstate *estate,
					PLpgSQL_function *func)
{
	int			ndatums = estate->ndatums;
	PLpgSQL_datum **indatums;
	PLpgSQL_datum **outdatums;
	char	   *workspace;
	char	   *ws_next;
	int			i;

	/* Allocate local datum-pointer array */
	estate->datums = palloc_array(PLpgSQL_datum *, ndatums);

	/*
	 * To reduce palloc overhead, we make a single palloc request for all the
	 * space needed for locally-instantiated datums.
	 */
	workspace = palloc(func->copiable_size);
	ws_next = workspace;

	/* Fill datum-pointer array, copying datums into workspace as needed */
	indatums = func->datums;
	outdatums = estate->datums;
	for (i = 0; i < ndatums; i++)
	{
		PLpgSQL_datum *indatum = indatums[i];
		PLpgSQL_datum *outdatum;

		/* This must agree with plpgsql_finish_datums on what is copiable */
		switch (indatum->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
			case PLPGSQL_DTYPE_PROMISE:
				outdatum = (PLpgSQL_datum *) ws_next;
				memcpy(outdatum, indatum, sizeof(PLpgSQL_var));
				ws_next += MAXALIGN(sizeof(PLpgSQL_var));
				break;

			case PLPGSQL_DTYPE_REC:
				outdatum = (PLpgSQL_datum *) ws_next;
				memcpy(outdatum, indatum, sizeof(PLpgSQL_rec));
				ws_next += MAXALIGN(sizeof(PLpgSQL_rec));
				break;

			case PLPGSQL_DTYPE_ROW:
			case PLPGSQL_DTYPE_RECFIELD:

				/*
				 * These datum records are read-only at runtime, so no need to
				 * copy them (well, RECFIELD contains cached data, but we'd
				 * just as soon centralize the caching anyway).
				 */
				outdatum = indatum;
				break;

			default:
				elog(ERROR, "unrecognized dtype: %d", indatum->dtype);
				outdatum = NULL;	/* keep compiler quiet */
				break;
		}

		outdatums[i] = outdatum;
	}

	Assert(ws_next == workspace + func->copiable_size);
}
