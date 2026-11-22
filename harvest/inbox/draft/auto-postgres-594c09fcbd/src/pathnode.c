// AUTO-DRAFT from postgres/postgres PR #9b5d4e3331753b3399e84965093109cac8f364bf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
 *	  memory consumption.  We dare not try to free the substructure of a Path,
 *	  since much of it may be shared with other Paths or the query tree itself;
 *	  but just recycling discarded Path nodes is a very useful savings in
 *	  a large join tree.  We can recycle the List nodes of pathlist, too.
 *
 *	  As noted in optimizer/README, deleting a previously-accepted Path is
 *	  safe because we know that Paths of this rel cannot yet be referenced
/* …（同文件无关代码省略）… */
 *	  referenced as children of BitmapHeapPaths as well as being paths in
 *	  their own right.  Hence, we don't pfree IndexPaths when rejecting them.
 *
 * 'parent_rel' is the relation entry to which the path corresponds.
 * 'new_path' is a potential path for parent_rel.
 *
/* …（同文件无关代码省略）… */
		{
			parent_rel->pathlist = foreach_delete_current(parent_rel->pathlist,
														  p1);

			/*
			 * Delete the data pointed-to by the deleted cell, if possible
			 */
			if (!IsA(old_path, IndexPath))
				pfree(old_path);
		}
		else
/* …（同文件无关代码省略）… */
	}
	else
	{
		/* Reject and recycle the new path */
		if (!IsA(new_path, IndexPath))
			pfree(new_path);
	}
}
/* …（同文件无关代码省略）… */
 *	  another partial path; this requires that there be no other references to
 *	  such paths yet.  Hence, GatherPaths must not be created for a rel until
 *	  we're done creating all partial paths for it.  Unlike add_path, we don't
 *	  take an exception for IndexPaths as partial index paths won't be
 *	  referenced by partial BitmapHeapPaths.
 */
void
add_partial_path(RelOptInfo *parent_rel, Path *new_path)
/* …（同文件无关代码省略）… */
		{
			parent_rel->partial_pathlist =
				foreach_delete_current(parent_rel->partial_pathlist, p1);
			pfree(old_path);
		}
		else
		{
/* …（同文件无关代码省略）… */
	}
	else
	{
		/* Reject and recycle the new path */
		pfree(new_path);
	}
}
