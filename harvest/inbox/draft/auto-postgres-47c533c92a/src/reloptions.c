// AUTO-DRAFT from postgres/postgres PR #6a3da13d376f5950fd105eedddc4a1e829e6b398
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
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
Datum
transformRelOptions(Datum oldOptions, List *defList, const char *nameSpace,
					const char *const validnsps[], bool acceptOidsOff, bool isReset)
{
	Datum		result;
	ArrayBuildState *astate;
	ListCell   *cell;

	/* no change if empty list */
	if (defList == NIL)
		return oldOptions;

	/* We build new array using accumArrayResult */
	astate = NULL;

	/* Copy any oldOptions that aren't to be replaced */
	if (DatumGetPointer(oldOptions) != NULL)
	{
		ArrayType  *array = DatumGetArrayTypeP(oldOptions);
		Datum	   *oldoptions;
		int			noldoptions;
		int			i;

		deconstruct_array_builtin(array, TEXTOID, &oldoptions, NULL, &noldoptions);

		for (i = 0; i < noldoptions; i++)
		{
			char	   *text_str = VARDATA(DatumGetPointer(oldoptions[i]));
			int			text_len = VARSIZE(DatumGetPointer(oldoptions[i])) - VARHDRSZ;

			/* Search for a match in defList */
			foreach(cell, defList)
			{
				DefElem    *def = (DefElem *) lfirst(cell);
				int			kw_len;

				/* ignore if not in the same namespace */
				if (nameSpace == NULL)
				{
					if (def->defnamespace != NULL)
						continue;
				}
				else if (def->defnamespace == NULL)
					continue;
				else if (strcmp(def->defnamespace, nameSpace) != 0)
					continue;

				kw_len = strlen(def->defname);
				if (text_len > kw_len && text_str[kw_len] == '=' &&
					strncmp(text_str, def->defname, kw_len) == 0)
					break;
			}
			if (!cell)
			{
				/* No match, so keep old option */
				astate = accumArrayResult(astate, oldoptions[i],
										  false, TEXTOID,
										  CurrentMemoryContext);
			}
		}
	}

	/*
	 * If CREATE/SET, add new options to array; if RESET, just check that the
	 * user didn't say RESET (option=val).  (Must do this because the grammar
	 * doesn't enforce it.)
	 */
	foreach(cell, defList)
	{
		DefElem    *def = (DefElem *) lfirst(cell);

		if (isReset)
		{
			if (def->arg != NULL)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("RESET must not include values for parameters")));
		}
		else
		{
			const char *name;
			const char *value;
			text	   *t;
			Size		len;

			/*
			 * Error out if the namespace is not valid.  A NULL namespace is
			 * always valid.
			 */
			if (def->defnamespace != NULL)
			{
				bool		valid = false;
				int			i;

				if (validnsps)
				{
					for (i = 0; validnsps[i]; i++)
					{
						if (strcmp(def->defnamespace, validnsps[i]) == 0)
						{
							valid = true;
							break;
						}
					}
				}

				if (!valid)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("unrecognized parameter namespace \"%s\"",
									def->defnamespace)));
			}

			/* ignore if not in the same namespace */
			if (nameSpace == NULL)
			{
				if (def->defnamespace != NULL)
					continue;
			}
			else if (def->defnamespace == NULL)
				continue;
			else if (strcmp(def->defnamespace, nameSpace) != 0)
				continue;

			/*
			 * Flatten the DefElem into a text string like "name=arg". If we
			 * have just "name", assume "name=true" is meant.  Note: the
			 * namespace is not output.
			 */
			name = def->defname;
			if (def->arg != NULL)
				value = defGetString(def);
			else
				value = "true";

			/* Insist that name not contain "=", else "a=b=c" is ambiguous */
			if (strchr(name, '=') != NULL)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid option name \"%s\": must not contain \"=\"",
								name)));

			/*
			 * This is not a great place for this test, but there's no other
			 * convenient place to filter the option out. As WITH (oids =
			 * false) will be removed someday, this seems like an acceptable
			 * amount of ugly.
			 */
			if (acceptOidsOff && def->defnamespace == NULL &&
				strcmp(name, "oids") == 0)
			{
				if (defGetBoolean(def))
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("tables declared WITH OIDS are not supported")));
				/* skip over option, reloptions machinery doesn't know it */
				continue;
			}

			len = VARHDRSZ + strlen(name) + 1 + strlen(value);
			/* +1 leaves room for sprintf's trailing null */
			t = (text *) palloc(len + 1);
			SET_VARSIZE(t, len);
			sprintf(VARDATA(t), "%s=%s", name, value);

			astate = accumArrayResult(astate, PointerGetDatum(t),
									  false, TEXTOID,
									  CurrentMemoryContext);
		}
	}

	if (astate)
		result = makeArrayResult(astate, CurrentMemoryContext);
	else
		result = (Datum) 0;

	return result;
}
/* …（同文件无关代码省略）… */
	return result;
}

/*
 * Storage for per-attribute version in the backbranches.
 *
 * See the "Version storage in the catalog" section in
 * src/backend/catalog/README.attver.md.
 */

/*
 * Extract the attribute version from an attoptions datum.  Returns
 * InvalidXLogRecPtr if the datum is null or does not carry a version (e.g. a
 * pg_upgrade'd or older-minor-version row).
 */
XLogRecPtr
attoptions_get_version(Datum attoptions, bool isnull)
{
	XLogRecPtr	version = InvalidXLogRecPtr;
	Datum	   *optiondatums;
	int			noptions;

	if (isnull || DatumGetPointer(attoptions) == NULL)
		return InvalidXLogRecPtr;

	/*
	 * Open code untransformRelOptions(), it seems unnecessary to do the full
	 * transformation here, as that would introduce more memory allocations.
	 * The code ends up being similarly long as not open-coding.
	 */

	deconstruct_array_builtin(DatumGetArrayTypeP(attoptions), TEXTOID,
							  &optiondatums, NULL, &noptions);

	for (int i = 0; i < noptions; i++)
	{
		char	   *optname;
		char	   *optval;

		optname = TextDatumGetCString(optiondatums[i]);
		optval = strchr(optname, '=');

		if (optval)
		{
			*optval++ = '\0';

			if (strcmp(optname, ATTRIBUTE_VERSION_ATTOPTION) == 0)
			{
				char	   *endptr;

				errno = 0;
				version = strtou64(optval, &endptr, 10);

				/*
				 * Don't accept -, any range errors, require whole string to
				 * be consumed. This should never be reached, but we shouldn't
				 * just ignore errors.
				 */
				if (!isdigit((unsigned char) *optval) || endptr == optval ||
					*endptr != 0 || errno != 0)
					elog(ERROR, "could not parse attribute version \"%s\"", optval);
				pfree(optname);
				break;
			}
		}
		pfree(optname);
	}

	pfree(optiondatums);

	return version;
}

/*
 * Return a new attoptions text[] datum with the attribute version set, or
 * removed if version is InvalidXLogRecPtr.  Other options already present in
 * attoptions (if not isnull) are kept; any previous version is replaced.
 */
Datum
attoptions_set_version(Datum attoptions, bool isnull, XLogRecPtr version)
{
	DefElem    *def;
	List	   *defList;
	String	   *str;
	bool		isReset = XLogRecPtrIsInvalid(version);

	/*
	 * Using the full reloptions machinery here isn't the cheapest, but
	 * compared to the cost of DDL it should not matter.
	 */

	if (isReset)
		str = NULL;
	else
		str = makeString(psprintf("%llu", (long long unsigned) version));

	def = makeDefElem(ATTRIBUTE_VERSION_ATTOPTION, (Node *) str, -1);
	defList = list_make1(def);

	return transformRelOptions(isnull ? (Datum) 0 : attoptions,
							   defList, NULL, NULL, false, isReset);
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
			numoptions++;

	if (numoptions > 0)
	{
		reloptions = palloc_array(relopt_value, numoptions);

		for (i = 0, j = 0; relOpts[i]; i++)
		{
			if (relOpts[i]->kinds & kind)
			{
				reloptions[j].gen = relOpts[i];
				reloptions[j].isset = false;
				j++;
			}
		}
	}

	/* Done if no options */
	if (DatumGetPointer(options) != NULL)
		parseRelOptionsInternal(options, validate, reloptions, numoptions);

	*numrelopts = numoptions;
	return reloptions;
}
/* …（同文件无关代码省略）… */
static void
parse_one_reloption(relopt_value *option, char *text_str, int text_len,
					bool validate)
{
	char	   *value;
	int			value_len;
	bool		parsed;
	bool		nofree = false;

	if (option->isset && validate)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("parameter \"%s\" specified more than once",
						option->gen->name)));

	value_len = text_len - option->gen->namelen - 1;
	value = (char *) palloc(value_len + 1);
	memcpy(value, text_str + option->gen->namelen + 1, value_len);
	value[value_len] = '\0';

	switch (option->gen->type)
	{
		case RELOPT_TYPE_BOOL:
			{
				parsed = parse_bool(value, &option->bool_val);
				if (validate && !parsed)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("invalid value for boolean option \"%s\": %s",
									option->gen->name, value)));
			}
			break;
		case RELOPT_TYPE_TERNARY:
			{
				bool		b;

				parsed = parse_bool(value, &b);
				option->ternary_val = b ? PG_TERNARY_TRUE :
					PG_TERNARY_FALSE;
				if (validate && !parsed)
					ereport(ERROR,
							errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("invalid value for boolean option \"%s\": %s",
								   option->gen->name, value));
			}
			break;
		case RELOPT_TYPE_INT:
			{
				relopt_int *optint = (relopt_int *) option->gen;

				parsed = parse_int(value, &option->int_val, 0, NULL);
				if (validate && !parsed)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("invalid value for integer option \"%s\": %s",
									option->gen->name, value)));
				if (validate && (option->int_val < optint->min ||
								 option->int_val > optint->max))
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("value %s out of bounds for option \"%s\"",
									value, option->gen->name),
							 errdetail("Valid values are between \"%d\" and \"%d\".",
									   optint->min, optint->max)));
			}
			break;
		case RELOPT_TYPE_REAL:
			{
				relopt_real *optreal = (relopt_real *) option->gen;

				parsed = parse_real(value, &option->real_val, 0, NULL);
				if (validate && !parsed)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("invalid value for floating point option \"%s\": %s",
									option->gen->name, value)));
				if (validate && (option->real_val < optreal->min ||
								 option->real_val > optreal->max))
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("value %s out of bounds for option \"%s\"",
									value, option->gen->name),
							 errdetail("Valid values are between \"%f\" and \"%f\".",
									   optreal->min, optreal->max)));
			}
			break;
		case RELOPT_TYPE_ENUM:
			{
				relopt_enum *optenum = (relopt_enum *) option->gen;
				relopt_enum_elt_def *elt;

				parsed = false;
				for (elt = optenum->members; elt->string_val; elt++)
				{
					if (pg_strcasecmp(value, elt->string_val) == 0)
					{
						option->enum_val = elt->symbol_val;
						parsed = true;
						break;
					}
				}
				if (validate && !parsed)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("invalid value for enum option \"%s\": %s",
									option->gen->name, value),
							 optenum->detailmsg ?
							 errdetail_internal("%s", _(optenum->detailmsg)) : 0));

				/*
				 * If value is not among the allowed string values, but we are
				 * not asked to validate, just use the default numeric value.
				 */
				if (!parsed)
					option->enum_val = optenum->default_val;
			}
			break;
		case RELOPT_TYPE_STRING:
			{
				relopt_string *optstring = (relopt_string *) option->gen;

				option->string_val = value;
				nofree = true;
				if (validate && optstring->validate_cb)
					(optstring->validate_cb) (value);
				parsed = true;
			}
			break;
		default:
			elog(ERROR, "unsupported reloption type %d", option->gen->type);
			parsed = true;		/* quiet compiler */
			break;
	}

	if (parsed)
		option->isset = true;
	if (!nofree)
		pfree(value);
}
/* …（同文件无关代码省略）… */
static void *
allocateReloptStruct(Size base, relopt_value *options, int numoptions)
{
	Size		size = base;
	int			i;

	for (i = 0; i < numoptions; i++)
	{
		relopt_value *optval = &options[i];

		if (optval->gen->type == RELOPT_TYPE_STRING)
		{
			relopt_string *optstr = (relopt_string *) optval->gen;

			if (optstr->fill_cb)
			{
				const char *val = optval->isset ? optval->string_val :
					optstr->default_isnull ? NULL : optstr->default_val;

				size += optstr->fill_cb(val, NULL);
			}
			else
				size += GET_STRING_RELOPTION_LEN(*optval) + 1;
		}
	}

	return palloc0(size);
}
/* …（同文件无关代码省略）… */
static void
fillRelOptions(void *rdopts, Size basesize,
			   relopt_value *options, int numoptions,
			   bool validate,
			   const relopt_parse_elt *elems, int numelems)
{
	int			i;
	int			offset = basesize;

	for (i = 0; i < numoptions; i++)
	{
		int			j;
		bool		found = false;

		for (j = 0; j < numelems; j++)
		{
			if (strcmp(options[i].gen->name, elems[j].optname) == 0)
			{
				relopt_string *optstring;
				char	   *itempos = ((char *) rdopts) + elems[j].offset;
				char	   *string_val;

				switch (options[i].gen->type)
				{
					case RELOPT_TYPE_BOOL:
						*(bool *) itempos = options[i].isset ?
							options[i].bool_val :
							((relopt_bool *) options[i].gen)->default_val;
						break;
					case RELOPT_TYPE_TERNARY:
						*(pg_ternary *) itempos = options[i].isset ?
							options[i].ternary_val : PG_TERNARY_UNSET;
						break;
					case RELOPT_TYPE_INT:
						*(int *) itempos = options[i].isset ?
							options[i].int_val :
							((relopt_int *) options[i].gen)->default_val;
						break;
					case RELOPT_TYPE_REAL:
						*(double *) itempos = options[i].isset ?
							options[i].real_val :
							((relopt_real *) options[i].gen)->default_val;
						break;
					case RELOPT_TYPE_ENUM:
						*(int *) itempos = options[i].isset ?
							options[i].enum_val :
							((relopt_enum *) options[i].gen)->default_val;
						break;
					case RELOPT_TYPE_STRING:
						optstring = (relopt_string *) options[i].gen;
						if (options[i].isset)
							string_val = options[i].string_val;
						else if (!optstring->default_isnull)
							string_val = optstring->default_val;
						else
							string_val = NULL;

						if (optstring->fill_cb)
						{
							Size		size =
								optstring->fill_cb(string_val,
												   (char *) rdopts + offset);

							if (size)
							{
								*(int *) itempos = offset;
								offset += size;
							}
							else
								*(int *) itempos = 0;
						}
						else if (string_val == NULL)
							*(int *) itempos = 0;
						else
						{
							strcpy((char *) rdopts + offset, string_val);
							*(int *) itempos = offset;
							offset += strlen(string_val) + 1;
						}
						break;
					default:
						elog(ERROR, "unsupported reloption type %d",
							 options[i].gen->type);
						break;
				}
				found = true;
				break;
			}
		}
		if (validate && !found)
			elog(ERROR, "reloption \"%s\" not found in parse table",
				 options[i].gen->name);
	}
	SET_VARSIZE(rdopts, offset);
}
/* …（同文件无关代码省略）… */
void *
build_reloptions(Datum reloptions, bool validate,
				 relopt_kind kind,
				 Size relopt_struct_size,
				 const relopt_parse_elt *relopt_elems,
				 int num_relopt_elems)
{
	int			numoptions;
	relopt_value *options;
	void	   *rdopts;

	/* parse options specific to given relation option kind */
	options = parseRelOptions(reloptions, validate, kind, &numoptions);
	Assert(numoptions <= num_relopt_elems);

	/* if none set, we're done */
	if (numoptions == 0)
	{
		Assert(options == NULL);
		return NULL;
	}

	/* allocate and fill the structure */
	rdopts = allocateReloptStruct(relopt_struct_size, options, numoptions);
	fillRelOptions(rdopts, relopt_struct_size, options, numoptions,
				   validate, relopt_elems, num_relopt_elems);

	pfree(options);

	return rdopts;
}
/* …（同文件无关代码省略）… */
		{"n_distinct_inherited", RELOPT_TYPE_REAL, offsetof(AttributeOpts, n_distinct_inherited)}
	};

	/*
	 * The reserved attribute version key is stored inside attoptions in the
	 * back branches, but it is not a user-visible / settable option.  Strip
	 * it before parsing so it is neither validated nor rejected as an
	 * unrecognized option.
	 */
	if (DatumGetPointer(reloptions) != NULL)
		reloptions = attoptions_set_version(reloptions, false,
											InvalidXLogRecPtr);

	return (bytea *) build_reloptions(reloptions, validate,
									  RELOPT_KIND_ATTRIBUTE,
									  sizeof(AttributeOpts),
