// AUTO-DRAFT from postgres/postgres PR #b84ad0cd90414970a453e37983624a13921421ca
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

		 * to find any plan at all.  We have to consider not only rels that
		 * are directly on the inner side of an OJ with the joinrel, but also
		 * ones that are indirectly so, so search to find all such rels.
		 */
		join_lateral_rels = min_join_parameterization(root, joinrelids,
													  rel1, rel2);
/* …（同文件无关代码省略）… */
						more = true;
					}
				}
			} while (more);
			if (bms_overlap(join_plus_rhs, join_lateral_rels))
				return false;	/* will not be able to join to some RHS rel */
