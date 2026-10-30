// AUTO-DRAFT from postgres/postgres PR #328a9a1731292e57a03ec4a5f495d0ed5fc85fba
#define SPTEST(f, x, y) \
	DatumGetBool(DirectFunctionCall2(f, PointPGetDatum(x), PointPGetDatum(y)))
/* …（同文件无关代码省略）… */
static int
getAxisSide(float8 tst, float8 centroid)
{
	/*
	 * Test FPeq first so that points on a centroid axis get the same result
	 * as getQuadrant.
	 */
	if (FPeq(tst, centroid))
		return 0;
	if (FPgt(tst, centroid))
		return 1;
	if (FPlt(tst, centroid))
		return -1;
	/* fall back to exact comparison */
	if (tst > centroid)
		return 1;
	if (tst < centroid)
		return -1;
  // <<< BUG ANCHOR
	elog(ERROR, "getQuadrant: impossible case");
	return 0;
}
/* …（同文件无关代码省略）… */
static int16
getQuadrant(Point *centroid, Point *tst)
{
	int			xside;
	int			yside;

	if ((SPTEST(point_above, tst, centroid) ||
		 SPTEST(point_horiz, tst, centroid)) &&
		(SPTEST(point_right, tst, centroid) ||
		 SPTEST(point_vert, tst, centroid)))
		return 1;

	if (SPTEST(point_below, tst, centroid) &&
		(SPTEST(point_right, tst, centroid) ||
		 SPTEST(point_vert, tst, centroid)))
		return 2;

	if ((SPTEST(point_below, tst, centroid) ||
		 SPTEST(point_horiz, tst, centroid)) &&
		SPTEST(point_left, tst, centroid))
		return 3;

	if (SPTEST(point_above, tst, centroid) &&
		SPTEST(point_left, tst, centroid))
		return 4;

	/*
	 * The fuzzy tests above can all fail on one axis, because FPeq() rounds a
	 * difference while FPlt() and FPgt() round a sum.  Compare exactly only
	 * on that axis.  On the other axis, keep the result of the fuzzy tests,
	 * because searches route by that result too.
	 */
	xside = getAxisSide(tst->x, centroid->x);
	yside = getAxisSide(tst->y, centroid->y);

	if (xside >= 0)
		return (yside >= 0) ? 1 : 2;
	return (yside <= 0) ? 3 : 4;
}
/* …（同文件无关代码省略）… */
					r |= 1 << getQuadrant(centroid, &p);
					p = boxQuery->high;
					r |= 1 << getQuadrant(centroid, &p);
					p.x = boxQuery->low.x;
					r |= 1 << getQuadrant(centroid, &p);

					which &= r;
