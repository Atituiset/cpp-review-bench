// AUTO-DRAFT from postgres/postgres PR #0d6103e537e1299046e5906149f9a7d2108be2cf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
									child_domain->jd_relids);
				jtitem->qualscope = bms_union(left_item->qualscope,
											  right_item->qualscope);
				/* caution: ANTI join derived from SEMI will lack rtindex */
				if (j->rtindex != 0)
				{
					parent_domain->jd_relids =
						bms_add_member(parent_domain->jd_relids,
									   j->rtindex);
					jtitem->qualscope = bms_add_member(jtitem->qualscope,
													   j->rtindex);
					root->outer_join_rels = bms_add_member(root->outer_join_rels,
														   j->rtindex);
					mark_rels_nulled_by_join(root, j->rtindex,
											 right_item->qualscope);
				}
				jtitem->inner_join_rels = bms_union(left_item->inner_join_rels,
													right_item->inner_join_rels);
				jtitem->left_rels = left_item->qualscope;
/* …（同文件无关代码省略）… */
static void
mark_rels_nulled_by_join(PlannerInfo *root, Index ojrelid,
						 Relids lower_rels)
{
	int			relid = -1;

	while ((relid = bms_next_member(lower_rels, relid)) > 0)
	{
		RelOptInfo *rel = root->simple_rel_array[relid];

		/* ignore the RTE_GROUP RTE */
		if (relid == root->group_rtindex)
			continue;

		if (rel == NULL)		/* must be an outer join */
		{
			Assert(bms_is_member(relid, root->outer_join_rels));
			continue;
		}
		rel->nulling_relids = bms_add_member(rel->nulling_relids, ojrelid);
	}
}
