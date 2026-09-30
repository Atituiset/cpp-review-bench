// AUTO-DRAFT from postgres/postgres PR #dca6a9e320e0272f0ea8d7e076cda04b06050866
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "optimizer/prep.h"
#include "optimizer/restrictinfo.h"
#include "parser/parse_agg.h"
#include "rewrite/rewriteManip.h"
#include "utils/lsyscache.h"

/*
 * Utility structure.  A sorting procedure is needed to simplify the search
 * of SJE-candidate baserels referencing the same database relation.  Having
 * collected all baserels from the query jointree, the planner sorts them
 * according to the reloid value, groups them with the next pass and attempts
 * to remove self-joins.
 *
 * Preliminary sorting prevents quadratic behavior that can be harmful in the
 * case of numerous joins.
 */
typedef struct
{
	int			relid;
	Oid			reloid;
} SelfJoinCandidate;

bool		enable_self_join_elimination;
/* …（同文件无关代码省略）… */
		return removed;			/* ... but don't fail to report sub-removals */

	/*
	 * In order to find relations with the same oid we first build an array of
	 * candidates and then sort it by oid.
	 */
	candidates = palloc_array(SelfJoinCandidate, numRels);
	i = -1;
	j = 0;
	while ((i = bms_next_member(relids, i)) >= 0)
	{
		candidates[j].relid = i;
		candidates[j].reloid = root->simple_rte_array[i]->relid;
		j++;
	}

	qsort(candidates, numRels, sizeof(SelfJoinCandidate),
		  self_join_candidates_cmp);

	/*
	 * Iteratively form a group of relation indexes with the same oid and
	 * launch the routine that detects self-joins in this group.
	 *
	 * We remove considered relations from relids as we scan, so that that set
/* …（同文件无关代码省略）… */
	i = 0;
	for (j = 1; j <= numRels; j++)
	{
		if (j == numRels || candidates[j].reloid != candidates[i].reloid)
		{
			if (j - i >= 2)
			{
				/* Create a group of relation indexes with the same oid */
				Relids		group = NULL;

				while (i < j)
/* …（同文件无关代码省略）… */
}

/*
 * Compare self-join candidates by their oids.
 */
static int
self_join_candidates_cmp(const void *a, const void *b)
{
	const SelfJoinCandidate *ca = (const SelfJoinCandidate *) a;
	const SelfJoinCandidate *cb = (const SelfJoinCandidate *) b;

	if (ca->reloid != cb->reloid)
		return (ca->reloid < cb->reloid ? -1 : 1);
	else
		return 0;
}
