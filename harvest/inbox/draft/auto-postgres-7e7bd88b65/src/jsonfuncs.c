// AUTO-DRAFT from postgres/postgres PR #b58a9a97fe783c8ce897fa1a5da23c59234aa6bf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <limits.h>

#include "access/htup_details.h"
#include "access/tupdesc.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "common/int.h"
#include "common/jsonapi.h"
/* …（同文件无关代码省略）… */
void
json_categorize_type(Oid typoid, bool is_jsonb,
					 JsonTypeCategory *tcategory, Oid *outfuncoid)
{
	bool		typisvarlena;

	/* Look through any domain */
	typoid = getBaseType(typoid);

	*outfuncoid = InvalidOid;

	switch (typoid)
	{
		case BOOLOID:
			*outfuncoid = F_BOOLOUT;
			*tcategory = JSONTYPE_BOOL;
			break;

		case INT2OID:
		case INT4OID:
		case INT8OID:
		case FLOAT4OID:
		case FLOAT8OID:
		case NUMERICOID:
			getTypeOutputInfo(typoid, outfuncoid, &typisvarlena);
			*tcategory = JSONTYPE_NUMERIC;
			break;

		case DATEOID:
			*outfuncoid = F_DATE_OUT;
			*tcategory = JSONTYPE_DATE;
			break;

		case TIMESTAMPOID:
			*outfuncoid = F_TIMESTAMP_OUT;
			*tcategory = JSONTYPE_TIMESTAMP;
			break;

		case TIMESTAMPTZOID:
			*outfuncoid = F_TIMESTAMPTZ_OUT;
			*tcategory = JSONTYPE_TIMESTAMPTZ;
			break;

		case JSONOID:
			getTypeOutputInfo(typoid, outfuncoid, &typisvarlena);
			*tcategory = JSONTYPE_JSON;
			break;

		case JSONBOID:
			getTypeOutputInfo(typoid, outfuncoid, &typisvarlena);
			*tcategory = is_jsonb ? JSONTYPE_JSONB : JSONTYPE_JSON;
			break;

		default:
			/* Check for arrays and composites */
			if (OidIsValid(get_element_type(typoid)) || typoid == ANYARRAYOID
				|| typoid == ANYCOMPATIBLEARRAYOID || typoid == RECORDARRAYOID)
			{
				*outfuncoid = F_ARRAY_OUT;
				*tcategory = JSONTYPE_ARRAY;
			}
			else if (type_is_rowtype(typoid))	/* includes RECORDOID */
			{
				*outfuncoid = F_RECORD_OUT;
				*tcategory = JSONTYPE_COMPOSITE;
			}
			else
			{
				/*
				 * It's probably the general case.  But let's look for a cast
				 * to json (note: not to jsonb even if is_jsonb is true), if
				 * it's not built-in.
				 */
				*tcategory = JSONTYPE_OTHER;
				if (typoid >= FirstNormalObjectId)
				{
					Oid			castfunc;
					CoercionPathType ctype;

					ctype = find_coercion_pathway(JSONOID, typoid,
												  COERCION_EXPLICIT,
												  &castfunc);
					if (ctype == COERCION_PATH_FUNC && OidIsValid(castfunc))
					{
						*outfuncoid = castfunc;
						*tcategory = JSONTYPE_CAST;
					}
					else
					{
						/* non builtin type with no cast */
						getTypeOutputInfo(typoid, outfuncoid, &typisvarlena);
					}
				}
				else
				{
					/* any other builtin type */
					getTypeOutputInfo(typoid, outfuncoid, &typisvarlena);
				}
			}
			break;
	}
}

/*
 * Check whether a type conversion to JSON or JSONB involves any mutable
 * functions.  This recurses into container types (arrays, composites,
 * ranges, multiranges, domains) to check their element/sub types.
 *
 * The caller must initialize *has_mutable to false before calling.
 * If any mutable function is found, *has_mutable is set to true.
 */
void
json_check_mutability(Oid typoid, bool is_jsonb, bool *has_mutable)
{
	char		att_typtype = get_typtype(typoid);
	JsonTypeCategory tcategory;
	Oid			outfuncoid;

	/* since this function recurses, it could be driven to stack overflow */
	check_stack_depth();

	Assert(has_mutable != NULL);

	if (*has_mutable)
		return;

	if (att_typtype == TYPTYPE_DOMAIN)
	{
		json_check_mutability(getBaseType(typoid), is_jsonb, has_mutable);
		return;
	}
	else if (att_typtype == TYPTYPE_COMPOSITE)
	{
		/*
		 * For a composite type, recurse into its attributes.  Use the
		 * typcache to avoid opening the relation directly.
		 */
		TupleDesc	tupdesc = lookup_rowtype_tupdesc(typoid, -1);

		for (int i = 0; i < tupdesc->natts; i++)
		{
			Form_pg_attribute attr = TupleDescAttr(tupdesc, i);

			if (attr->attisdropped)
				continue;

			json_check_mutability(attr->atttypid, is_jsonb, has_mutable);
			if (*has_mutable)
				break;
		}
		ReleaseTupleDesc(tupdesc);
		return;
	}
	else if (att_typtype == TYPTYPE_RANGE)
	{
		json_check_mutability(get_range_subtype(typoid), is_jsonb,
							  has_mutable);
		return;
	}
	else if (att_typtype == TYPTYPE_MULTIRANGE)
	{
		json_check_mutability(get_multirange_range(typoid), is_jsonb,
							  has_mutable);
		return;
	}
	else
	{
		Oid			att_typelem = get_element_type(typoid);

		if (OidIsValid(att_typelem))
		{
			/* recurse into array element type */
			json_check_mutability(att_typelem, is_jsonb, has_mutable);
			return;
		}
	}

	json_categorize_type(typoid, is_jsonb, &tcategory, &outfuncoid);

	switch (tcategory)
	{
		case JSONTYPE_NULL:
		case JSONTYPE_BOOL:
		case JSONTYPE_NUMERIC:
			break;

		case JSONTYPE_DATE:
		case JSONTYPE_TIMESTAMP:
		case JSONTYPE_TIMESTAMPTZ:
			*has_mutable = true;
			break;

		case JSONTYPE_JSON:
		case JSONTYPE_JSONB:
		case JSONTYPE_ARRAY:
		case JSONTYPE_COMPOSITE:
			break;

		case JSONTYPE_CAST:
		case JSONTYPE_OTHER:
			if (func_volatile(outfuncoid) != PROVOLATILE_IMMUTABLE)
				*has_mutable = true;
			break;
	}
}
