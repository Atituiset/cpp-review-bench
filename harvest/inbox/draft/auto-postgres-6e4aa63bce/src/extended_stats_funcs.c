// AUTO-DRAFT from postgres/postgres PR #87524d0dd8470819ce8aa3708a9bf3fa3f1d34fd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
static bool check_all_expr_argnames_valid(JsonbContainer *cont, AttrNumber exprnum);
static Datum array_in_safe(FmgrInfo *array_in, const char *s, Oid typid,
						   int32 typmod, AttrNumber exprnum,
						   const char *element_name, bool *ok);
static Datum import_pg_statistic(Relation pgsd, JsonbContainer *cont,
								 AttrNumber exprnum, FmgrInfo *array_in_fn,
								 Oid typid, int32 typmod, Oid typcoll,
/* …（同文件无关代码省略）… */
static bool
key_in_expr_argnames(JsonbValue *key)
{
	Assert(key->type == jbvString);
	for (int i = 0; i < NUM_ATTRIBUTE_STATS_ELEMS; i++)
	{
		if (strlen(extexprargname[i]) == key->val.string.len &&
			strncmp(extexprargname[i], key->val.string.val, key->val.string.len) == 0)
			return true;
	}
	return false;
}
/* …（同文件无关代码省略）… */
static bool
check_all_expr_argnames_valid(JsonbContainer *cont, AttrNumber exprnum)
{
	bool		all_keys_valid = true;

	JsonbIterator *jbit;
	JsonbIteratorToken jitok;
	JsonbValue	jkey;

	Assert(JsonContainerIsObject(cont));

	jbit = JsonbIteratorInit(cont);

	/* We always start off with a BEGIN OBJECT */
	jitok = JsonbIteratorNext(&jbit, &jkey, false);
	Assert(jitok == WJB_BEGIN_OBJECT);

	while (true)
	{
		JsonbValue	jval;

		jitok = JsonbIteratorNext(&jbit, &jkey, false);

		/*
		 * We have run of keys. This is the only condition where it is
		 * memory-safe to break out of the loop.
		 */
		if (jitok == WJB_END_OBJECT)
			break;

		/* We can only find keys inside an object */
		Assert(jitok == WJB_KEY);
		Assert(jkey.type == jbvString);

		/* A value must follow the key.  Skip nested containers. */
		jitok = JsonbIteratorNext(&jbit, &jval, true);
		Assert(jitok == WJB_VALUE);

		/*
		 * If we have already found an invalid key, there is no point in
		 * looking for more, because additional WARNINGs are just clutter. We
		 * must continue iterating over the json to ensure that we clean up
		 * all allocated memory.
		 */
		if (!all_keys_valid)
			continue;

		if (!key_in_expr_argnames(&jkey))
		{
			ereport(WARNING,
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("could not import element in expression %d: invalid key name",
						   exprnum));
			all_keys_valid = false;
		}
	}
	return all_keys_valid;
}
/* …（同文件无关代码省略）… */
static char *
jbv_string_get_cstr(JsonbValue *jval)
{
	char	   *s;

	Assert(jval->type == jbvString);

	s = palloc0(jval->val.string.len + 1);
	memcpy(s, jval->val.string.val, jval->val.string.len);

	return s;
}
/* …（同文件无关代码省略）… */
 *
 * If an error is encountered, capture it, and reduce the elevel to WARNING.
 *
 * This is an adaptation of statatt_build_stavalues().
 */
static Datum
array_in_safe(FmgrInfo *array_in, const char *s, Oid typid, int32 typmod,
			  AttrNumber exprnum, const char *element_name, bool *ok)
{
	Datum		result;

	ErrorSaveContext escontext = {
		.type = T_ErrorSaveContext,
		.details_wanted = true
	};

	*ok = false;

	/*
	 * If the array_in function returned an error, we will want to report that
	 * ERROR as a WARNING, and add some location context to the error message.
	 * Overwriting the existing hint (if any) is not ideal, and an error
	 * context would only work for level >= ERROR.
	 */
	if (!InputFunctionCallSafe(array_in, s, typid, typmod,
							   (Node *) &escontext, &result))
	{
		StringInfoData hint_str;

		initStringInfo(&hint_str);
		appendStringInfo(&hint_str,
						 "Element \"%s\" in expression %d could not be parsed.",
						 element_name, exprnum);
		escontext.error_data->elevel = WARNING;
		escontext.error_data->hint = hint_str.data;
		ThrowErrorData(escontext.error_data);
		pfree(hint_str.data);
		return (Datum) 0;
	}

	if (ARR_NDIM(DatumGetArrayTypeP(result)) != 1)
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("could not import element \"%s\" in expression %d: must be a one-dimensional array",
						element_name, exprnum)));
		return (Datum) 0;
	}

	if (array_contains_nulls(DatumGetArrayTypeP(result)))
	{
		ereport(WARNING,
				errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("could not import element \"%s\" in expression %d: null value found",
					   element_name, exprnum));
		return (Datum) 0;
	}

	*ok = true;
	return result;
}
/* …（同文件无关代码省略）… */
		s = jbv_string_get_cstr(&val[MOST_COMMON_VALS_ELEM]);
		stavalues = array_in_safe(array_in_fn, s, typid, typmod, exprnum,
								  extexprargname[MOST_COMMON_VALS_ELEM],
								  &val_ok);

		pfree(s);

		s = jbv_string_get_cstr(&val[MOST_COMMON_FREQS_ELEM]);
		stanumbers = array_in_safe(array_in_fn, s, FLOAT4OID, -1, exprnum,
								   extexprargname[MOST_COMMON_FREQS_ELEM],
								   &num_ok);
		pfree(s);

		/* Only set the slot if both datums have been built */
/* …（同文件无关代码省略）… */

		stavalues = array_in_safe(array_in_fn, s, typid, typmod, exprnum,
								  extexprargname[HISTOGRAM_BOUNDS_ELEM],
								  &val_ok);
		pfree(s);

		if (val_ok)
/* …（同文件无关代码省略）… */
		s = jbv_string_get_cstr(&val[MOST_COMMON_ELEMS_ELEM]);
		stavalues = array_in_safe(array_in_fn, s, elemtypid, typmod, exprnum,
								  extexprargname[MOST_COMMON_ELEMS_ELEM],
								  &val_ok);
		pfree(s);


		s = jbv_string_get_cstr(&val[MOST_COMMON_ELEM_FREQS_ELEM]);
		stanumbers = array_in_safe(array_in_fn, s, FLOAT4OID, -1, exprnum,
								   extexprargname[MOST_COMMON_ELEM_FREQS_ELEM],
								   &num_ok);
		pfree(s);

		/* Only set the slot if both datums have been built */
/* …（同文件无关代码省略）… */
		s = jbv_string_get_cstr(&val[ELEM_COUNT_HISTOGRAM_ELEM]);
		stanumbers = array_in_safe(array_in_fn, s, FLOAT4OID, -1, exprnum,
								   extexprargname[ELEM_COUNT_HISTOGRAM_ELEM],
								   &num_ok);
		pfree(s);

		if (num_ok)
/* …（同文件无关代码省略）… */

		stavalues = array_in_safe(array_in_fn, s, rtypid, typmod, exprnum,
								  extexprargname[RANGE_BOUNDS_HISTOGRAM_ELEM],
								  &val_ok);

		if (val_ok && statatt_check_bounds_histogram(stavalues))
			statatt_set_slot(values, nulls, replaces,
/* …（同文件无关代码省略）… */
		s = jbv_string_get
