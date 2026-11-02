// AUTO-DRAFT from postgres/postgres PR #303c9b36205d11e41c7d5d61a1802ec50bccbf5a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>


			if (splan->plan->extParam != NULL)
				UpdateChangedParamSet(splan, node->chgParam);
		}
		/* Well. Now set chgParam for child trees. */
		if (outerPlanState(node) != NULL)
