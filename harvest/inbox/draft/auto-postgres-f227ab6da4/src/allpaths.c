// AUTO-DRAFT from postgres/postgres PR #abda2a2c9b2a76687513ef223f782b333951149f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

		 * If set_append_rel_size() decided the parent appendrel was
		 * parallel-unsafe at some point after visiting this child rel, we
		 * need to propagate the unsafety marking down to the child, so that
		 * we don't generate useless partial paths for it.
		 */
		if (!rel->consider_parallel)
			childrel->consider_parallel = false;

		/*
		 * Compute the child's access paths.
