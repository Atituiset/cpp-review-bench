// AUTO-DRAFT from postgres/postgres PR #627c4df3b73ae00a0dfd08f4216514fb9ea44b5d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

					{
						val_string = DatumGetCString(DirectFunctionCall1(jsonb_out,
																		 JsonbPGetDatum(JsonbValueToJsonb(jbv))));
					}
					else if (jsexpr->use_json_coercion)
					{
/* …（同文件无关代码省略）… */
					}
					else
					{
						val_string = ExecGetJsonValueItemString(jbv, op->resnull);

						/*
/* …（同文件无关代码省略）… */
static char *
ExecGetJsonValueItemString(JsonbValue *item, bool *resnull)
{
	*resnull = false;

	/* get coercion state reference and datum of the corresponding SQL type */
	switch (item->type)
	{
		case jbvNull:
			*resnull = true;
			return NULL;

		case jbvString:
			{
				char	   *str = palloc(item->val.string.len + 1);

				memcpy(str, item->val.string.val, item->val.string.len);
				str[item->val.string.len] = '\0';
				return str;
			}

		case jbvNumeric:
			return DatumGetCString(DirectFunctionCall1(numeric_out,
													   NumericGetDatum(item->val.numeric)));

		case jbvBool:
			return DatumGetCString(DirectFunctionCall1(boolout,
													   BoolGetDatum(item->val.boolean)));

		case jbvDatetime:
			switch (item->val.datetime.typid)
			{
				case DATEOID:
					return DatumGetCString(DirectFunctionCall1(date_out,
															   item->val.datetime.value));
				case TIMEOID:
					return DatumGetCString(DirectFunctionCall1(time_out,
															   item->val.datetime.value));
				case TIMETZOID:
					return DatumGetCString(DirectFunctionCall1(timetz_out,
															   item->val.datetime.value));
				case TIMESTAMPOID:
					return DatumGetCString(DirectFunctionCall1(timestamp_out,
															   item->val.datetime.value));
				case TIMESTAMPTZOID:
					return DatumGetCString(DirectFunctionCall1(timestamptz_out,
															   item->val.datetime.value));
				default:
					elog(ERROR, "unexpected jsonb datetime type oid %u",
						 item->val.datetime.typid);
			}
			break;

		case jbvArray:
		case jbvObject:
		case jbvBinary:
			return DatumGetCString(DirectFunctionCall1(jsonb_out,
													   JsonbPGetDatum(JsonbValueToJsonb(item))));

		default:
			elog(ERROR, "unexpected jsonb value type %d", item->type);
	}

	Assert(false);
	*resnull = true;
	return NULL;
}
