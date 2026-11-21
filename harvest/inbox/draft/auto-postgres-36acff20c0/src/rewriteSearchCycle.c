// AUTO-DRAFT from postgres/postgres PR #65ebed1f5e2f16d56234b5a27b8d508d18c3854d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

		newsubquery->targetList = lappend(newsubquery->targetList, tle);
	}

	newrte->subquery = newsubquery;
	newrte->inFromCl = true;
	newq2->rtable = list_make1(newrte);
