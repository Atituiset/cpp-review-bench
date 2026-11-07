// AUTO-DRAFT from postgres/postgres PR #4a934833fb02ac2a18d0a3f86b67b493d7fe8979
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

				 * handle it using any NOT MATCHED [BY TARGET] actions.
				 * Otherwise, for a NOT MATCHED BY SOURCE action, neither the
				 * source row nor the target row now exists, so there is no
				 * futher action to execute.
				 */
				if (relaction->mas_action->matchKind == MERGE_WHEN_MATCHED)
					*matched = false;
/* …（同文件无关代码省略）… */
							 * using any NOT MATCHED [BY TARGET] actions.
							 * Otherwise, for a NOT MATCHED BY SOURCE action,
							 * neither the source row nor the target row now
							 * exists, so there is no futher action to
							 * execute.
							 */
							if (was_matched)
