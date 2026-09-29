// AUTO-DRAFT from postgres/postgres PR #6a93535798aa9219d01c01c67161a78780e2af1a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
typedef struct NumericSumAccum
{
	int			ndigits;
	int			weight;
	int			dscale;
	int			num_uncarried;
	bool		have_carry_space;
	int32	   *pos_digits;
	int32	   *neg_digits;
} NumericSumAccum;
/* …（同文件无关代码省略）… */
typedef struct NumericAggState
{
	bool		calcSumX2;		/* if true, calculate sumX2 */
	MemoryContext agg_context;	/* context we're calculating in */
	int64		N;				/* count of processed numbers */
	NumericSumAccum sumX;		/* sum of processed numbers */
	NumericSumAccum sumX2;		/* sum of squares of processed numbers */
	int			maxScale;		/* maximum scale seen so far */
	int64		maxScaleCount;	/* number of values seen with maximum scale */
	/* These counts are *not* included in N!  Use NA_TOTAL_COUNT() as needed */
	int64		NaNcount;		/* count of NaN values */
	int64		pInfcount;		/* count of +Inf values */
	int64		nInfcount;		/* count of -Inf values */
} NumericAggState;
/* …（同文件无关代码省略）… */
static NumericAggState *
makeNumericAggStateCurrentContext(bool calcSumX2)
{
	NumericAggState *state;

	state = palloc0_object(NumericAggState);
	state->calcSumX2 = calcSumX2;
	state->agg_context = CurrentMemoryContext;

	return state;
}
/* …（同文件无关代码省略）… */
	initReadOnlyStringInfo(&buf, VARDATA_ANY(sstate),
						   VARSIZE_ANY_EXHDR(sstate));

	result = makeNumericAggStateCurrentContext(false);

	/* N */
	result->N = pq_getmsgint64(&buf);
/* …（同文件无关代码省略）… */
typedef struct Int128AggState
{
	bool		calcSumX2;		/* if true, calculate sumX2 */
	int64		N;				/* count of processed numbers */
	INT128		sumX;			/* sum of processed numbers */
	INT128		sumX2;			/* sum of squares of processed numbers */
} Int128AggState;
/* …（同文件无关代码省略）… */
static Int128AggState *
makeInt128AggStateCurrentContext(bool calcSumX2)
{
	Int128AggState *state;

	state = palloc0_object(Int128AggState);
	state->calcSumX2 = calcSumX2;

	return state;
}
/* …（同文件无关代码省略）… */
	initReadOnlyStringInfo(&buf, VARDATA_ANY(sstate),
						   VARSIZE_ANY_EXHDR(sstate));

	result = makeInt128AggStateCurrentContext(false);

	/* N */
	result->N = pq_getmsgint64(&buf);
