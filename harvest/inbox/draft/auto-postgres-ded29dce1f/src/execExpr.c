// AUTO-DRAFT from postgres/postgres PR #edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
			}
			Assert(pertrans->numTransInputs == argno);
		}
		else if (pertrans->numInputs == 1)
		{
			/*
			 * Non-presorted DISTINCT and/or ORDER BY case, with a single
			 * column sorted on.
			 */
			TargetEntry *source_tle =
				(TargetEntry *) linitial(pertrans->aggref->args);
/* …（同文件无关代码省略）… */
		{
			/*
			 * Non-presorted DISTINCT and/or ORDER BY case, with multiple
			 * columns sorted on.
			 */
			Datum	   *values = pertrans->sortslot->tts_values;
			bool	   *nulls = pertrans->sortslot->tts_isnull;
/* …（同文件无关代码省略）… */
				scratch->opcode = EEOP_AGG_PLAIN_TRANS_BYREF;
		}
	}
	else if (pertrans->numInputs == 1)
		scratch->opcode = EEOP_AGG_ORDERED_TRANS_DATUM;
	else
		scratch->opcode = EEOP_AGG_ORDERED_TRANS_TUPLE;
