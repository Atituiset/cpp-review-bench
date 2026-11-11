// AUTO-DRAFT from postgres/postgres PR #c5daeb1e0f5adfbc19ed621bb97ce2d68c8455bf
#include "utils/formatting.h"
#include "utils/json.h"
#include "utils/jsonpath.h"
#include "utils/memutils.h"
#include "utils/timestamp.h"
  // <<< BUG ANCHOR
/* …（同文件无关代码省略）… */
			break;
		case TEXTOID:
		case VARCHAROID:
			{
				text	   *txt = DatumGetTextPP(val);

				res->type = jbvString;
				res->val.string.val = VARDATA_ANY(txt);
				res->val.string.len = VARSIZE_ANY_EXHDR(txt);
				break;
			}
		case DATEOID:
		case TIMEOID:
		case TIMETZOID:
/* …（同文件无关代码省略）… */
				break;
			}
		default:
			ereport(ERROR,
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("could not convert value of type %s to jsonpath",
						   format_type_be(typid)));
	}
}
