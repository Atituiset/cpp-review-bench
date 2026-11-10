// AUTO-DRAFT from postgres/postgres PR #6a3da13d376f5950fd105eedddc4a1e829e6b398
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
#endif
}

/*
 * Attribute versions, stored after the attribute arrays in the back branches.
 * versions[0] is the aggregate version, versions[1..natts] are the
 * per-attribute versions.  See "Version storage in tuple descriptors" in
 * src/backend/catalog/README.attver.md.
 */
typedef struct TupleDescVersionData
{
	uint64		magic;			/* TUPLEDESC_VERSION_MAGIC */
	XLogRecPtr	versions[FLEXIBLE_ARRAY_MEMBER];
} TupleDescVersionData;

#define TUPLEDESC_VERSION_MAGIC UINT64CONST(0xDE5C00000001DE5C)

/* size of the version storage for a descriptor with natts attributes */
static inline Size
TupleDescVersionDataSize(int natts)
{
	return offsetof(TupleDescVersionData, versions) +
		sizeof(XLogRecPtr) * (natts + 1);
}

static inline TupleDescVersionData *
TupleDescGetVersionData(TupleDesc tupdesc)
{
	Size		off = offsetof(struct TupleDescData, compact_attrs) +
		tupdesc->natts * sizeof(CompactAttribute) +
		tupdesc->natts * sizeof(FormData_pg_attribute);

	return (TupleDescVersionData *) ((char *) tupdesc + MAXALIGN(off));
}

/*
 * Address of the attribute version array.  A descriptor not allocated by
 * CreateTemplateTupleDesc() doesn't have one, which the magic number
 * detects.
 */
static inline XLogRecPtr *
TupleDescVersionArray(TupleDesc tupdesc)
{
	TupleDescVersionData *vdata = TupleDescGetVersionData(tupdesc);

	if (unlikely(vdata->magic != TUPLEDESC_VERSION_MAGIC))
		elog(ERROR, "tuple descriptor for type %u lacks attribute versions",
			 tupdesc->tdtypeid);

	return vdata->versions;
}

/*
 * CreateTemplateTupleDesc
 *		This function allocates an empty tuple descriptor structure.
/* …（同文件无关代码省略）… */
CreateTemplateTupleDesc(int natts)
{
	TupleDesc	desc;
	TupleDescVersionData *vdata;
	size_t		sz;

	/*
/* …（同文件无关代码省略）… */
		natts * sizeof(CompactAttribute) +
		natts * sizeof(FormData_pg_attribute);

	/* Storage for TupleDescVersionData */
	sz = MAXALIGN(sz) + TupleDescVersionDataSize(natts);

	desc = (TupleDesc) palloc(sz);

	/*
/* …（同文件无关代码省略）… */
	desc->tdrefcount = -1;		/* assume not reference-counted */

	/*
	 * Zero-initialize the per-attribute and aggregate versions. RECORD
	 * descriptors aren't versioned, and we don't know if this is going to be
	 * used for something else, so we have to zero it here.
	 */
	vdata = TupleDescGetVersionData(desc);
	memset(vdata, 0, TupleDescVersionDataSize(natts));
	vdata->magic = TUPLEDESC_VERSION_MAGIC;

	/* This will be set to the correct value by TupleDescFinalize() */
	desc->firstNonCachedOffsetAttr = -1;
/* …（同文件无关代码省略）… */
	return desc;
}

/*
 * CopyTupleDescVersions
 *		Copy the attribute versions from src into dst.
 *
 * dst may have fewer attributes than src (a truncated copy), in which case
 * the aggregate version becomes that of dst's last attribute, preserving the
 * invariant that the aggregate is the last attribute's version.
 */
static void
CopyTupleDescVersions(TupleDesc dst, TupleDesc src)
{
	Assert(src->tdtypeid != RECORDOID);

	if (dst->natts == 0)
	{
		TupleDescSetVersion(dst, InvalidXLogRecPtr);
		return;
	}

	for (int i = 1; i <= dst->natts; i++)
		TupleDescSetAttrVersion(dst, i, TupleDescGetAttrVersion(src, i));

	TupleDescSetVersion(dst, TupleDescGetAttrVersion(src, dst->natts));
}

/*
 * CreateTupleDescCopy
 *		This function creates a new TupleDesc by copying from an existing
/* …（同文件无关代码省略）… */
	desc->tdtypeid = tupdesc->tdtypeid;
	desc->tdtypmod = tupdesc->tdtypmod;

	if (tupdesc->tdtypeid != RECORDOID)
		CopyTupleDescVersions(desc, tupdesc);

	TupleDescFinalize(desc);

	return desc;
/* …（同文件无关代码省略）… */
	desc->tdtypeid = tupdesc->tdtypeid;
	desc->tdtypmod = tupdesc->tdtypmod;

	if (tupdesc->tdtypeid != RECORDOID)
		CopyTupleDescVersions(desc, tupdesc);

	TupleDescFinalize(desc);

	return desc;
/* …（同文件无关代码省略）… */
	desc->tdtypeid = tupdesc->tdtypeid;
	desc->tdtypmod = tupdesc->tdtypmod;

	if (tupdesc->tdtypeid != RECORDOID)
		CopyTupleDescVersions(desc, tupdesc);

	TupleDescFinalize(desc);

	return desc;
/* …（同文件无关代码省略）… */
/*
 * TupleDescCopySize
 *		Return the number of bytes TupleDescCopySized() needs to copy src.
 *
 * In the back branches this includes the attribute version storage, so that
 * the copy is laid out like a descriptor allocated by
 * CreateTemplateTupleDesc().
 */
Size
TupleDescCopySize(TupleDesc src)
{
	return MAXALIGN(TupleDescSize(src)) + TupleDescVersionDataSize(src->natts);
}

/*
 * Implementation of TupleDescCopy() and TupleDescCopySized(): flat-copy the
 * header and attribute arrays of src into dst.
 */
static void
TupleDescCopyInternal(TupleDesc dst, TupleDesc src)
{
	memcpy(dst, src, TupleDescSize(src));

	/*
	 * Since we're not copying constraints and defaults, clear fields
/* …（同文件无关代码省略）… */
	TupleDescFinalize(dst);
}

/*
 * TupleDescCopySized
 *		Copy a tuple descriptor into caller-supplied memory of dstsize bytes.
 *		The memory may be shared memory mapped at any address, and must
 *		be sufficient to hold TupleDescCopySize(src) bytes.
 *
 * !!! Constraints and defaults are not copied !!!
 */
void
TupleDescCopySized(TupleDesc dst, Size dstsize, TupleDesc src)
{
	Size		sz = TupleDescCopySize(src);
	TupleDescVersionData *tvdata;

	if (dstsize < sz)
		elog(ERROR, "insufficient space to copy tuple descriptor: %zu bytes, need %zu",
			 dstsize, sz);

	/* Flat-copy the header and attribute arrays */
	TupleDescCopyInternal(dst, src);

	/*
	 * Fill in the version storage.  A !RECORDOID source has to have version
	 * storage, copy it.  RECORDOID descriptors aren't versioned, and the
	 * source could theoretically be a copy made with TupleDescCopy(), lacking
	 * the version storage; don't read it and initialize it as
	 * CreateTemplateTupleDesc() does.
	 */
	tvdata = TupleDescGetVersionData(dst);
	if (src->tdtypeid != RECORDOID)
	{
		/* verifies that the source has a valid array */
		(void) TupleDescVersionArray(src);
		memcpy(tvdata, TupleDescGetVersionData(src),
			   TupleDescVersionDataSize(src->natts));
