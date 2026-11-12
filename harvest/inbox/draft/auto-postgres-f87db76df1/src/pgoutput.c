// AUTO-DRAFT from postgres/postgres PR #9a849767eb281c9502aab4a7384a654004ec6ffe
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
		ListCell   *lc;
		Oid			publish_as_relid = relid;
		int			publish_ancestor_level = 0;
		bool		am_partition = get_rel_relispartition(relid);
		char		relkind = get_rel_relkind(relid);
		List	   *rel_publications = NIL;

		/* Reload publications if needed before use. */
		if (!publications_valid)
/* …（同文件无关代码省略）… */
			{
				List	   *exceptpubids = NIL;

				if (am_partition)
				{
					List	   *ancestors = get_partition_ancestors(relid);
					Oid			last_ancestor_relid = llast_oid(ancestors);

					/*
/* …（同文件无关代码省略）… */
				 * published via this publication, which will be used as the
				 * relation via which to publish the partition's changes.
				 */
				if (am_partition)
				{
					Oid			ancestor;
					int			level;
					List	   *ancestors = get_partition_ancestors(relid);

					ancestor = GetTopMostAncestorInPublication(pub->oid,
															   ancestors,
/* …（同文件无关代码省略）… */
		list_free(pubids);
		list_free(schemaPubids);
		list_free(rel_publications);

		entry->replicate_valid = true;
	}
