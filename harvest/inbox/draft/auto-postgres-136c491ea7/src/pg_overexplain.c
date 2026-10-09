// AUTO-DRAFT from postgres/postgres PR #38475f23c83358eafe42a410e4342eb8d64ba24c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

			ExplainOpenGroup("Elided Node", NULL, true, es);
			ExplainPropertyText("Elided Node Type", elidednodetag, es);
			overexplain_bitmapset("Elided Node RTIs", n->relids, es);
			ExplainCloseGroup("Elided Node", NULL, true, es);
		}
		if (opened_elided_nodes)
/* …（同文件无关代码省略）… */
static void
overexplain_bitmapset(const char *qlabel, Bitmapset *bms, ExplainState *es)
{
	int			x = -1;

	StringInfoData buf;

	if (bms_is_empty(bms))
	{
		ExplainPropertyText(qlabel, "none", es);
		return;
	}

	initStringInfo(&buf);
	while ((x = bms_next_member(bms, x)) >= 0)
		appendStringInfo(&buf, " %d", x);
	Assert(buf.data[0] == ' ');
	ExplainPropertyText(qlabel, buf.data + 1, es);
	pfree(buf.data);
}
