// AUTO-DRAFT from postgres/postgres PR #6d4607e886a083f7cddb121a84f6d8ee23addd6d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>

#include "catalog/pg_statistic_ext.h"
#include "catalog/pg_statistic_ext_data.h"
#include "statistics/extended_stats_internal.h"
#include "utils/syscache.h"
#include "utils/typcache.h"
#include "varatt.h"
/* …（同文件无关代码省略）… */
typedef struct CombinationGenerator
{
	int			k;				/* size of the combination */
	int			n;				/* total number of elements */
	int			current;		/* index of the next combination to return */
	int			ncombinations;	/* number of combinations (size of array) */
	int		   *combinations;	/* array of pre-built combinations */
} CombinationGenerator;
/* …（同文件无关代码省略）… */
	uint32		itemcnt;
	int			numattrs = data->nattnums;
	int			numcombs = num_combinations(numattrs);

	result = palloc(offsetof(MVNDistinct, items) +
					numcombs * sizeof(MVNDistinctItem));
	result->magic = STATS_NDISTINCT_MAGIC;
	result->type = STATS_NDISTINCT_TYPE_BASIC;
	result->nitems = numcombs;

	itemcnt = 0;
	for (k = 2; k <= numattrs; k++)
	{
/* …（同文件无关代码省略）… */
		{
			MVNDistinctItem *item = &result->items[itemcnt];
			int			j;

			item->attributes = palloc_array(AttrNumber, k);
			item->nattributes = k;
/* …（同文件无关代码省略）… */
				Assert(AttributeNumberIsValid(item->attributes[j]));
			}

			item->ndistinct =
				ndistinct_for_combination(totalrows, data, k, combination);

			itemcnt++;
			Assert(itemcnt <= result->nitems);
/* …（同文件无关代码省略）… */
		generator_free(generator);
	}

	/* must consume exactly the whole output array */
	Assert(itemcnt == result->nitems);

/* …（同文件无关代码省略）… */
static double
ndistinct_for_combination(double totalrows, StatsBuildData *data,
						  int k, int *combination)
{
	int			i,
				j;
	int			f1,
				cnt,
				d;
	bool	   *isnull;
	Datum	   *values;
	SortItem   *items;
	MultiSortSupport mss;
	int			numrows = data->numrows;

	mss = multi_sort_init(k);

	/*
	 * In order to determine the number of distinct elements, create separate
	 * values[]/isnull[] arrays with all the data we have, then sort them
	 * using the specified column combination as dimensions.  We could try to
	 * sort in place, but it'd probably be more complex and bug-prone.
	 */
	items = palloc_array(SortItem, numrows);
	values = palloc0_array(Datum, numrows * k);
	isnull = palloc0_array(bool, numrows * k);

	for (i = 0; i < numrows; i++)
	{
		items[i].values = &values[i * k];
		items[i].isnull = &isnull[i * k];
	}

	/*
	 * For each dimension, set up sort-support and fill in the values from the
	 * sample data.
	 *
	 * We use the column data types' default sort operators and collations;
	 * perhaps at some point it'd be worth using column-specific collations?
	 */
	for (i = 0; i < k; i++)
	{
		Oid			typid;
		TypeCacheEntry *type;
		Oid			collid = InvalidOid;
		VacAttrStats *colstat = data->stats[combination[i]];

		typid = colstat->attrtypid;
		collid = colstat->attrcollid;

		type = lookup_type_cache(typid, TYPECACHE_LT_OPR);
		if (type->lt_opr == InvalidOid) /* shouldn't happen */
			elog(ERROR, "cache lookup failed for ordering operator for type %u",
				 typid);

		/* prepare the sort function for this dimension */
		multi_sort_add_dimension(mss, i, type->lt_opr, collid);

		/* accumulate all the data for this dimension into the arrays */
		for (j = 0; j < numrows; j++)
		{
			items[j].values[i] = data->values[combination[i]][j];
			items[j].isnull[i] = data->nulls[combination[i]][j];
		}
	}

	/* We can sort the array now ... */
	qsort_interruptible(items, numrows, sizeof(SortItem),
						multi_sort_compare, mss);

	/* ... and count the number of distinct combinations */

	f1 = 0;
	cnt = 1;
	d = 1;
	for (i = 1; i < numrows; i++)
	{
		if (multi_sort_compare(&items[i], &items[i - 1], mss) != 0)
		{
			if (cnt == 1)
				f1 += 1;

			d++;
			cnt = 0;
		}

		cnt += 1;
	}

	if (cnt == 1)
		f1 += 1;

	return estimate_ndistinct(totalrows, numrows, d, f1);
}
/* …（同文件无关代码省略）… */
static double
estimate_ndistinct(double totalrows, int numrows, int d, int f1)
{
	double		numer,
				denom,
				ndistinct;

	numer = (double) numrows * (double) d;

	denom = (double) (numrows - f1) +
		(double) f1 * (double) numrows / totalrows;

	ndistinct = numer / denom;

	/* Clamp to sane range in case of roundoff error */
	if (ndistinct < (double) d)
		ndistinct = (double) d;

	if (ndistinct > totalrows)
		ndistinct = totalrows;

	return floor(ndistinct + 0.5);
}
/* …（同文件无关代码省略）… */
static int
num_combinations(int n)
{
	return (1 << n) - (n + 1);
}
/* …（同文件无关代码省略）… */
static void
generator_free(CombinationGenerator *state)
{
	pfree(state->combinations);
	pfree(state);
}
