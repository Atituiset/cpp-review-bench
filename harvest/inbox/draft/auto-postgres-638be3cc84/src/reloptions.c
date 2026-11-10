// AUTO-DRAFT from postgres/postgres PR #245eb832f88631c986a2dadad2cbe94f57089fd6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define GET_STRING_RELOPTION_LEN(option) \
	((option).isset ? strlen((option).string_val) : \
	 ((relopt_string *) (option).gen)->default_len)
/* …（同文件无关代码省略）… */
static void
assert_toast_defaults_unsettable(void)
{
	for (int i = 0; relOpts[i]; i++)
	{
		relopt_gen *gen = relOpts[i];

		if ((gen->kinds & RELOPT_KIND_TOAST) == 0)
			continue;

		/*
		 * A TOAST table's value is filled in from its main table's at the
		 * same offset in the same struct, so the option must be settable on a
		 * heap too.
		 */
		Assert((gen->kinds & RELOPT_KIND_HEAP) != 0);

		switch (gen->type)
		{
			case RELOPT_TYPE_TERNARY:

				/*
				 * Ternaries carry no default, and parse_one_reloption() can
				 * only produce true or false, so PG_TERNARY_UNSET is already
				 * beyond a user's reach.
				 */
				break;

			case RELOPT_TYPE_INT:
				{
					relopt_int *optint = (relopt_int *) gen;

					Assert(optint->default_val < optint->min ||
						   optint->default_val > optint->max);
					break;
				}

			case RELOPT_TYPE_REAL:
				{
					relopt_real *optreal = (relopt_real *) gen;

					Assert(optreal->default_val < optreal->min ||
						   optreal->default_val > optreal->max);
					break;
				}

			case RELOPT_TYPE_ENUM:
				{
					relopt_enum *optenum = (relopt_enum *) gen;

					for (relopt_enum_elt_def *elt = optenum->members;
						 elt->string_val; elt++)
						Assert(elt->symbol_val != optenum->default_val);
					break;
				}

			default:
				/* Neither bools nor strings can express "unset". */
				Assert(false);
		}
	}
}
/* …（同文件无关代码省略）… */
static void
initialize_reloptions(void)
{
	int			i;
	int			j;

	j = 0;
	for (i = 0; boolRelOpts[i].gen.name; i++)
	{
		Assert(DoLockModesConflict(boolRelOpts[i].gen.lockmode,
								   boolRelOpts[i].gen.lockmode));
		j++;
	}
	for (i = 0; ternaryRelOpts[i].gen.name; i++)
	{
		Assert(DoLockModesConflict(ternaryRelOpts[i].gen.lockmode,
								   ternaryRelOpts[i].gen.lockmode));
		j++;
	}

	for (i = 0; intRelOpts[i].gen.name; i++)
	{
		Assert(DoLockModesConflict(intRelOpts[i].gen.lockmode,
								   intRelOpts[i].gen.lockmode));
		j++;
	}
	for (i = 0; realRelOpts[i].gen.name; i++)
	{
		Assert(DoLockModesConflict(realRelOpts[i].gen.lockmode,
								   realRelOpts[i].gen.lockmode));
		j++;
	}
	for (i = 0; enumRelOpts[i].gen.name; i++)
	{
		Assert(DoLockModesConflict(enumRelOpts[i].gen.lockmode,
								   enumRelOpts[i].gen.lockmode));
		j++;
	}
	for (i = 0; stringRelOpts[i].gen.name; i++)
	{
		Assert(DoLockModesConflict(stringRelOpts[i].gen.lockmode,
								   stringRelOpts[i].gen.lockmode));
		j++;
	}
	j += num_custom_options;

	if (relOpts)
		pfree(relOpts);
	relOpts = MemoryContextAlloc(TopMemoryContext,
								 (j + 1) * sizeof(relopt_gen *));

	j = 0;
	for (i = 0; boolRelOpts[i].gen.name; i++)
	{
		relOpts[j] = &boolRelOpts[i].gen;
		relOpts[j]->type = RELOPT_TYPE_BOOL;
		relOpts[j]->namelen = strlen(relOpts[j]->name);
		j++;
	}

	for (i = 0; ternaryRelOpts[i].gen.name; i++)
	{
		relOpts[j] = &ternaryRelOpts[i].gen;
		relOpts[j]->type = RELOPT_TYPE_TERNARY;
		relOpts[j]->namelen = strlen(relOpts[j]->name);
		j++;
	}

	for (i = 0; intRelOpts[i].gen.name; i++)
	{
		relOpts[j] = &intRelOpts[i].gen;
		relOpts[j]->type = RELOPT_TYPE_INT;
		relOpts[j]->namelen = strlen(relOpts[j]->name);
		j++;
	}

	for (i = 0; realRelOpts[i].gen.name; i++)
	{
		relOpts[j] = &realRelOpts[i].gen;
		relOpts[j]->type = RELOPT_TYPE_REAL;
		relOpts[j]->namelen = strlen(relOpts[j]->name);
		j++;
	}

	for (i = 0; enumRelOpts[i].gen.name; i++)
	{
		relOpts[j] = &enumRelOpts[i].gen;
		relOpts[j]->type = RELOPT_TYPE_ENUM;
		relOpts[j]->namelen = strlen(relOpts[j]->name);
		j++;
	}

	for (i = 0; stringRelOpts[i].gen.name; i++)
	{
		relOpts[j] = &stringRelOpts[i].gen;
		relOpts[j]->type = RELOPT_TYPE_STRING;
		relOpts[j]->namelen = strlen(relOpts[j]->name);
		j++;
	}

	for (i = 0; i < num_custom_options; i++)
	{
		relOpts[j] = custom_options[i];
		j++;
	}

	/* add a list terminator */
	relOpts[j] = NULL;

	/* flag the work is complete */
	need_initialization = false;

#ifdef USE_ASSERT_CHECKING
	assert_toast_defaults_unsettable();
#endif
}
/* …（同文件无关代码省略）… */
	return result;
}

/*
 * Extract and parse reloptions from a pg_class tuple.
 *
/* …（同文件无关代码省略）… */
static void
parseRelOptionsInternal(Datum options, bool validate,
						relopt_value *reloptions, int numoptions)
{
	ArrayType  *array = DatumGetArrayTypeP(options);
	Datum	   *optiondatums;
	int			noptions;
	int			i;

	deconstruct_array_builtin(array, TEXTOID, &optiondatums, NULL, &noptions);

	for (i = 0; i < noptions; i++)
	{
		char	   *text_str = VARDATA(DatumGetPointer(optiondatums[i]));
		int			text_len = VARSIZE(DatumGetPointer(optiondatums[i])) - VARHDRSZ;
		int			j;

		/* Search for a match in reloptions */
		for (j = 0; j < numoptions; j++)
		{
			int			kw_len = reloptions[j].gen->namelen;

			if (text_len > kw_len && text_str[kw_len] == '=' &&
				strncmp(text_str, reloptions[j].gen->name, kw_len) == 0)
			{
				parse_one_reloption(&reloptions[j], text_str, text_len,
									validate);
				break;
			}
		}

		if (j >= numoptions && validate)
		{
			char	   *s;
			char	   *p;

			s = TextDatumGetCString(optiondatums[i]);
			p = strchr(s, '=');
			if (p)
				*p = '\0';
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("unrecognized parameter \"%s\"", s)));
		}
	}

	/* It's worth avoiding memory leaks in this function */
	pfree(optiondatums);

	if (((void *) array) != DatumGetPointer(options))
		pfree(array);
}
/* …（同文件无关代码省略）… */
static relopt_value *
parseRelOptions(Datum options, bool validate, relopt_kind kind,
				int *numrelopts)
{
	relopt_value *reloptions = NULL;
	int			numoptions = 0;
	int			i;
	int			j;

	if (need_initialization)
		initialize_reloptions();

	/* Build a list of expected options, based on kind */

	for (i = 0; relOpts[i]; i++)
		if (relOpts[i]->kinds & kind)
			num
