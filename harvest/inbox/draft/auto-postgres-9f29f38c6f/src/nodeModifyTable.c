// AUTO-DRAFT from postgres/postgres PR #7fffb114377507fca5e8c2b4db1820a4ec11615b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	 * Note that the planner does not build a withCheckOptionList or
	 * returningList for the root relation, but as in ExecInitPartitionInfo,
	 * we can use the first resultRelInfo entry as a reference to calculate
	 * the attno's for the root table.
	 */
	if (rootRelInfo != mtstate->resultRelInfo &&
		rootRelInfo->ri_RelationDesc->rd_rel->relkind != RELKIND_PARTITIONED_TABLE &&
/* …（同文件无关代码省略）… */
				part_attmap =
					build_attrmap_by_name(RelationGetDescr(rootRelation),
										  RelationGetDescr(firstResultRel),
										  false);

				wcoList = (List *)
					map_variable_attnos((Node *) wcoList,
/* …（同文件无关代码省略）… */
					part_attmap =
						build_attrmap_by_name(RelationGetDescr(rootRelation),
											  RelationGetDescr(firstResultRel),
											  false);

				returningList = (List *)
					map_variable_attnos((Node *) returningList,
