// AUTO-DRAFT from postgres/postgres PR #17fc99f4e50c878d56de798e80ebcbb7a99e312c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#include "utils/expandeddatum.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"


/*
/* …（同文件无关代码省略）… */
	((att)->attlen == -1 && (att)->attispackable)


/* GUC variable */
int			composite_type_mismatch = COMPOSITE_TYPE_MISMATCH_ERROR;

/* …（同文件无关代码省略）… */
{
	HeapTupleHeader td;

	/*
	 * TODO: Fetch the tupledesc for the datum tuple (if it's already one).
	 * Without that it's somewhat easy to have this hide a mismatched tuple.
	 */

	/*
	 * If the tuple contains any external TOAST pointers, we have to inline
	 * those fields to meet the conventions for composite-type Datums.
/* …（同文件无关代码省略）… */
void
heap_tuple_set_datum(HeapTupleHeader tup, TupleDesc tupleDesc, uint32 len)
{
	HeapTupleHeaderSetDatumLength(tup, len);
	HeapTupleHeaderSetTypeId(tup, tupleDesc->tdtypeid);
	HeapTupleHeaderSetDatum(tup);

	if (tupleDesc->tdtypeid == RECORDOID)
	{
		/*
		 * RECORDOID tuples are not protected by versioning (and do not need
		 * to be).
		 */
		HeapTupleHeaderSetTypMod(tup, tupleDesc->tdtypmod);

		/*
		 * Invalidating t_ctid makes the tuple an unversioned datum, even if
		 * it previously was a versioned one.
		 */
		ItemPointerSetInvalid(&tup->t_ctid);
	}
	else
	{
		int			natts = HeapTupleHeaderGetNatts(tup);
		XLogRecPtr	version;

		/*
		 * Stamp the datum with the version of the last attribute it actually
		 * contains, not the version of the last attribute in the tuple desc.
		 * That is important in case the tuple has fewer attributes than the
		 * descriptor.
		 *
		 * A zero-attribute tuple contains no data to decode, so it is always
		 * safe to deform; give it an explicit version of InvalidXLogRecPtr,
		 * so it can be deformed if columns are added later (as the version is
		 * stored in pg_attribute, a rowtype without columns has no version).
		 *
		 * It is never valid to have fewer attributes in the descriptor than
		 * the tuple when forming a tuple, so error out in that case.
		 * Hopefully unreachable.
		 */
		if (natts == 0)
			version = InvalidXLogRecPtr;
		else if (natts == tupleDesc->natts)
			version = TupleDescGetVersion(tupleDesc);
		else if (natts < tupleDesc->natts)
			version = TupleDescGetAttrVersion(tupleDesc, natts);
		else
			elog(ERROR, "datum's descriptor has fewer (%d) attributes than tuple (%d)",
				 tupleDesc->natts, natts);

		HeapTupleHeaderSetVersion(tup, version);
	}
}
/* …（同文件无关代码省略）… */
 * If the tuple already is a versioned datum, it is verified to be compatible
 * with tupleDesc first, as relabeling it would otherwise hide a
 * mismatch.
 */
void
heap_datum_update_type(HeapTupleHeader tup, TupleDesc tupleDesc)
{
	Assert(HeapTupleHeaderReallyIsDatum(tup));

	if (HeapTupleHeaderIsVersionedDatum(tup))
		heap_datum_check(tup, tupleDesc);

	heap_tuple_set_datum(tup, tupleDesc,
						 HeapTupleHeaderGetDatumLength(tup));
}

/*
 * Report accessing a tuple with an incompatible tuple descriptor, due to a
 * version mismatch.  descVersion is the version the tuple was compared with,
/* …（同文件无关代码省略）… */
pg_noinline static void
heap_datum_version_mismatch_error(Oid tupTypeid, Oid descTypeid,
								  XLogRecPtr tupVersion, XLogRecPtr descVersion)
{
	int			error_level =
		composite_type_mismatch == COMPOSITE_TYPE_MISMATCH_WARN ?
		WARNING : ERROR;

	/*
	 * NB: We can't embed the versions in the primary message without making
	 * test output unstable. Putting them into DETAIL would require the tests
	 * to use \set VERBOSITY terse, which would also hide the other DETAIL and
	 * CONTEXT lines that many of them benefit from. Therefore the versions
	 * are only reported in the server log. That's not pretty, but ...
	 */

	/* log details about the mismatch */
	ereport(LOG_SERVER_ONLY,
			errcode(ERRCODE_DATUM_VERSION_MISMATCH),
			errmsg("tuple descriptor for type %s (OID %u), version %X/%X is incompatible with tuple type %s (OID %u), version %X/%X",
				   format_type_be(descTypeid), descTypeid,
				   LSN_FORMAT_ARGS(descVersion),
				   format_type_be(tupTypeid), tupTypeid,
				   LSN_FORMAT_ARGS(tupVersion)));

	/* raise the actual error / warning */
	ereport(error_level,
			errcode(ERRCODE_DATUM_VERSION_MISMATCH),
			tupVersion < descVersion ?
			errmsg("row type of datum tuple was altered after the datum's creation")
			:
			errmsg("using outdated tuple descriptor when deforming row type"),
			errhint("See server log for additional details."));
}
/* …（同文件无关代码省略）… */
static bool
datum_attribute_version_ok(TupleDesc tupleDesc, int tupNatts,
						   XLogRecPtr tupVersion, XLogRecPtr *cmpVersion)
{
	*cmpVersion = TupleDescGetVersion(tupleDesc);

	/* should only be called if the aggregate version does not match */
	Assert(*cmpVersion != tupVersion);

	/* always safe to deform no columns */
	if (tupNatts == 0)
		return true;

	/*
	 * Iff the tuple is lacking columns in the descriptor, we can check the
	 * version of the last column present.
	 *
	 * We do not allow cases where the tuple descriptor has fewer columns than
	 * the tuple, as that indicates using a too old descriptor. It might be
	 * safe to allow that in some cases, but there don't seem to be any
	 * convincing cases that'd benefit.
	 */
	if (tupleDesc->natts > tupNatts)
	{
		*cmpVersion = TupleDescGetAttrVersion(tupleDesc, tupNatts);
		return tupVersion == *cmpVersion;
	}

	return false;
}

/*
 * Check that a versioned datum tuple can be safely decoded with the
 * tupleDesc, raise an error if not.
/* …（同文件无关代码省略）… */
 * This function implements the attribute versioning checks described in
 * src/backend/catalog/README.attver.md.  For partially historical reasons, it
 * needs to handle some corner cases, described below.
 */
void
heap_datum_check(HeapTupleHeader tup, TupleDesc tupleDesc)
{
	XLogRecPtr	descVersion = TupleDescGetVersion(tupleDesc);
	Oid			tupTypeid = HeapTupleHeaderGetTypeId(tup);
	int32		tupTypmod = HeapTupleHeaderGetTypMod(tup);
	XLogRecPtr	tupVersion;

	if (unlikely(composite_type_mismatch == COMPOSITE_TYPE_MISMATCH_DISABLED))
	
