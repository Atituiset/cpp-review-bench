// AUTO-DRAFT from postgres/postgres PR #a999971331301a4a2f945220733b11af90cc2962
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#include "access/htup_details.h"
#include "catalog/namespace.h"
#include "catalog/pg_type.h"
#include "mb/pg_wchar.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
/* …（同文件无关代码省略）… */
char *
format_type_extended(Oid type_oid, int32 typemod, uint16 flags)
{
	HeapTuple	tuple;
	Form_pg_type typeform;
	Oid			array_base_type;
	bool		is_array;
	char	   *buf;
	bool		with_typemod;

	if (type_oid == InvalidOid)
	{
		if ((flags & FORMAT_TYPE_INVALID_AS_NULL) != 0)
			return NULL;
		else if ((flags & FORMAT_TYPE_ALLOW_INVALID) != 0)
			return pstrdup("-");
	}

	tuple = SearchSysCache1(TYPEOID, ObjectIdGetDatum(type_oid));
	if (!HeapTupleIsValid(tuple))
	{
		if ((flags & FORMAT_TYPE_INVALID_AS_NULL) != 0)
			return NULL;
		else if ((flags & FORMAT_TYPE_ALLOW_INVALID) != 0)
			return pstrdup("???");
		else
			elog(ERROR, "cache lookup failed for type %u", type_oid);
	}
	typeform = (Form_pg_type) GETSTRUCT(tuple);

	/*
	 * Check if it's a "true" array type.  Pseudo-array types such as "name"
	 * shouldn't get deconstructed.  Also check the toast property, and don't
	 * deconstruct "plain storage" array types --- this is because we don't
	 * want to show oidvector as oid[].
	 */
	array_base_type = typeform->typelem;

	if (IsTrueArrayType(typeform) &&
		typeform->typstorage != TYPSTORAGE_PLAIN)
	{
		/* Switch our attention to the array element type */
		ReleaseSysCache(tuple);
		tuple = SearchSysCache1(TYPEOID, ObjectIdGetDatum(array_base_type));
		if (!HeapTupleIsValid(tuple))
		{
			if ((flags & FORMAT_TYPE_INVALID_AS_NULL) != 0)
				return NULL;
			else if ((flags & FORMAT_TYPE_ALLOW_INVALID) != 0)
				return pstrdup("???[]");
			else
				elog(ERROR, "cache lookup failed for type %u", type_oid);
		}
		typeform = (Form_pg_type) GETSTRUCT(tuple);
		type_oid = array_base_type;
		is_array = true;
	}
	else
		is_array = false;

	with_typemod = (flags & FORMAT_TYPE_TYPEMOD_GIVEN) != 0 && (typemod >= 0);

	/*
	 * See if we want to special-case the output for certain built-in types.
	 * Note that these special cases should all correspond to special
	 * productions in gram.y, to ensure that the type name will be taken as a
	 * system type, not a user type of the same name.
	 *
	 * If we do not provide a special-case output here, the type name will be
	 * handled the same way as a user type name --- in particular, it will be
	 * double-quoted if it matches any lexer keyword.  This behavior is
	 * essential for some cases, such as types "bit" and "char".
	 */
	buf = NULL;					/* flag for no special case */

	switch (type_oid)
	{
		case BITOID:
			if (with_typemod)
				buf = printTypmod("bit", typemod, typeform->typmodout);
			else if ((flags & FORMAT_TYPE_TYPEMOD_GIVEN) != 0)
			{
				/*
				 * bit with typmod -1 is not the same as BIT, which means
				 * BIT(1) per SQL spec.  Report it as the quoted typename so
				 * that parser will not assign a bogus typmod.
				 */
			}
			else
				buf = pstrdup("bit");
			break;

		case BOOLOID:
			buf = pstrdup("boolean");
			break;

		case BPCHAROID:
			if (with_typemod)
				buf = printTypmod("character", typemod, typeform->typmodout);
			else if ((flags & FORMAT_TYPE_TYPEMOD_GIVEN) != 0)
			{
				/*
				 * bpchar with typmod -1 is not the same as CHARACTER, which
				 * means CHARACTER(1) per SQL spec.  Report it as bpchar so
				 * that parser will not assign a bogus typmod.
				 */
			}
			else
				buf = pstrdup("character");
			break;

		case FLOAT4OID:
			buf = pstrdup("real");
			break;

		case FLOAT8OID:
			buf = pstrdup("double precision");
			break;

		case INT2OID:
			buf = pstrdup("smallint");
			break;

		case INT4OID:
			buf = pstrdup("integer");
			break;

		case INT8OID:
			buf = pstrdup("bigint");
			break;

		case NUMERICOID:
			if (with_typemod)
				buf = printTypmod("numeric", typemod, typeform->typmodout);
			else
				buf = pstrdup("numeric");
			break;

		case INTERVALOID:
			if (with_typemod)
				buf = printTypmod("interval", typemod, typeform->typmodout);
			else
				buf = pstrdup("interval");
			break;

		case TIMEOID:
			if (with_typemod)
				buf = printTypmod("time", typemod, typeform->typmodout);
			else
				buf = pstrdup("time without time zone");
			break;

		case TIMETZOID:
			if (with_typemod)
				buf = printTypmod("time", typemod, typeform->typmodout);
			else
				buf = pstrdup("time with time zone");
			break;

		case TIMESTAMPOID:
			if (with_typemod)
				buf = printTypmod("timestamp", typemod, typeform->typmodout);
			else
				buf = pstrdup("timestamp without time zone");
			break;

		case TIMESTAMPTZOID:
			if (with_typemod)
				buf = printTypmod("timestamp", typemod, typeform->typmodout);
			else
				buf = pstrdup("timestamp with time zone");
			break;

		case VARBITOID:
			if (with_typemod)
				buf = printTypmod("bit varying", typemod, typeform->typmodout);
			else
				buf = pstrdup("bit varying");
			break;

		case VARCHAROID:
			if (with_typemod)
				buf = printTypmod("character varying", typemod, typeform->typmodout);
			else
				buf = pstrdup("character varying");
			break;

		case JSONOID:
			buf = pstrdup("json");
			break;
	}

	if (buf == NULL)
	{
		/*
		 * Default handling: report the name as it appears in the catalog.
		 * Here, we must qualify the name if it is not visible in the search
		 * path or if caller requests it; and we must double-quote it if it's
		 * not a standard identifier or if it matches any keyword.
		 */
		char	   *nspname;
		char	   *typname;

		if ((flags & FORMAT_TYPE_FORCE_QUALIFY) == 0 &&
			TypeIsVisible(type_oid))
			nspname = NULL;
		else
			nspname = get_namespace_name_or_temp(typeform->typnamespace);

		typname = NameStr(typeform->typname);

		buf = quote_qualified_identifier(nspname, typname);

		if (with_typemod)
			buf = printTypmod(buf, typemod, typeform->typmodout);
	}

	if (is_array)
		buf = psprintf("%s[]", buf);

	ReleaseSysCache(tuple);

	return buf;
}
/* …（同文件无关代码省略）… */
static char *
printTypmod(const char *typname, int32 typmod, Oid typmodout)
{
	char	   *res;

	/* Shouldn't be called if typmod is -1 */
	Assert(typmod >= 0);

	if (typmodout == InvalidOid)
	{
		/* Default behavior: just print the integer typmod with parens */
		res = psprintf("%s(%d)", typname, typmod);
	}
	else
	{
		/* Use the type-specific typmodout procedure */
		char	   *tmstr;

		tmstr = DatumGetCString(OidFunctionCall1(typmodout,
												 Int32GetDatum(typmod)));
		res = psprintf("%s%s", typname, tmstr);
	}

	return res;
}
/* …（同文件无关代码省略）… */
oidvectortypes(PG_FUNCTION_ARGS)
{
	oidvector  *oidArray = (oidvector *) PG_GETARG_POINTER(0);
	char	   *result;
	int			numargs;
	int			num;
	size_t		total;
	size_t		left;

	/* validate input before fetching dim1 */
	check_valid_oidvector(oidArray);
	numargs = oidArray->dim1;

	total = 20 * numargs + 1;
	result = palloc(total);
	result[0] = '\0';
	left = total - 1;

	for (num = 0; num < numargs; num++)
	{
		char	   *typename = format_type_extended(oidArray->values[num], -1,
													FORMAT_TYPE_ALLOW_INVALID);
		size_t		slen = strlen(typename);

		if (left < (slen + 2))
		{
			total += slen + 2;
			result = repalloc(result, total);
			left += slen + 2;
		}

		if (num > 0)
		{
			strcat(result, ", ");
			left -= 2;
		}
		strcat(result, typename);
		left -= slen;
	}

	PG_RETURN_TEXT_P(cstring_to_text(result));
}
