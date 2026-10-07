// AUTO-DRAFT from postgres/postgres PR #e40b509c08f34e245bfab4124e24297ddaf4d535
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
			/*
			 * Adjust spilled_tuple so that it can be used as the new tuple in
			 * the update that we're about to replay.  This fixes TOAST
			 * pointers as well as remove useless values from dropped columns.
			 */
			prepare_concurrent_update(spilled_tuple, ondisk_tuple);

/* …（同文件无关代码省略）… */
{
	uint32		t_len;
	HeapTuple	tup;
	int			natt_ext;

	/* Read the tuple. */
/* …（同文件无关代码省略）… */
	 */
	ExecForceStoreHeapTuple(tup, slot, false);

	/*
	 * Next, read any attributes we stored separately into the tts_values
	 * array elements expecting them, if any.  This matches
/* …（同文件无关代码省略）… */
	BufFileReadExact(file, &natt_ext, sizeof(natt_ext));
	if (natt_ext > 0)
	{
		TupleDesc	desc = slot->tts_tupleDescriptor;

		for (int i = 0; i < desc->natts; i++)
		{
			CompactAttribute *attr = TupleDescCompactAttr(desc, i);
/* …（同文件无关代码省略）… */
 * update that we're about to replay.
 *
 * We perform the following critical change:
 * - Any EXTERNAL_ONDISK toast pointers so that it points to the corresponding
 *   toast value in 'src' (the transient table) instead.  The TOAST storage for
 *   'dest' is going to be dropped, so these values cannot be used any longer.
 *
 * We also apply the following optimization:
 * - If any columns are dropped but the slot still contains values, mark them
 *   as null to avoid uselessly wasting space in the new relation.
 */
static void
prepare_concurrent_update(TupleTableSlot *dest, TupleTableSlot *src)
{
	TupleDesc	desc = dest->tts_tupleDescriptor;

	for (int i = 0; i < desc->natts; i++)
	{
		CompactAttribute *attr = TupleDescCompactAttr(desc, i);
		varlena    *varlena_dst;

		if (attr->attisdropped)
		{
			if (!slot_attisnull(dest, i + 1))
			{
				slot_getsomeattrs(dest, i + 1);
				dest->tts_isnull[i] = true;
			}
			continue;
		}
		if (attr->attlen != -1)
			continue;
		if (slot_attisnull(dest, i + 1))
			continue;

		slot_getsomeattrs(dest, i + 1);

		varlena_dst = (varlena *) DatumGetPointer(dest->tts_values[i]);
		if (!VARATT_IS_EXTERNAL_ONDISK(varlena_dst))
			continue;
		slot_getsomeattrs(src, i + 1);

		dest->tts_values[i] = src->tts_values[i];
	}
}
