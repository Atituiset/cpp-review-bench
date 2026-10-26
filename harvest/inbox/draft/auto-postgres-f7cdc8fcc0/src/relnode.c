// AUTO-DRAFT from postgres/postgres PR #f9bc96514afc9c71d533568f62dc613be8356c4d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	return ppi;
}

/*
 * Returns a ParamPathInfo for the parameterization given by required_outer, if
 * already available in the given rel. Returns NULL otherwise.
/* …（同文件无关代码省略）… */
 * get_param_path_clause_serials
 *		Given a parameterized Path, return the set of pushed-down clauses
 *		(identified by rinfo_serial numbers) enforced within the Path.
 */
Bitmapset *
get_param_path_clause_serials(Path *path)
{
	if (path->param_info == NULL)
		return NULL;			/* not parameterized */

	/*
	 * We don't currently support parameterized MergeAppend paths, as
	 * explained in the comments for generate_orderedappend_paths.
	 */
	Assert(!IsA(path, MergeAppendPath));

	if (IsA(path, NestPath) ||
		IsA(path, MergePath) ||
		IsA(path, HashPath))
	{
		/*
		 * For a join path, combine clauses enforced within either input path
		 * with those enforced as joinrestrictinfo in this path.  Note that
		 * joinrestrictinfo may include some non-pushed-down clauses, but for
		 * current purposes it's okay if we include those in the result. (To
		 * be more careful, we could check for clause_relids overlapping the
		 * path parameterization, but it's not worth the cycles for now.)
		 */
		JoinPath   *jpath = (JoinPath *) path;
		Bitmapset  *pserials;
		ListCell   *lc;

		pserials = NULL;
		pserials = bms_add_members(pserials,
								   get_param_path_clause_serials(jpath->outerjoinpath));
		pserials = bms_add_members(pserials,
								   get_param_path_clause_serials(jpath->innerjoinpath));
		foreach(lc, jpath->joinrestrictinfo)
		{
			RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);

			pserials = bms_add_member(pserials, rinfo->rinfo_serial);
		}
		return pserials;
	}
	else if (IsA(path, AppendPath))
	{
		/*
		 * For an appendrel, take the intersection of the sets of clauses
		 * enforced in each input path.
		 */
		AppendPath *apath = (AppendPath *) path;
		Bitmapset  *pserials;
		ListCell   *lc;

		pserials = NULL;
		foreach(lc, apath->subpaths)
		{
			Path	   *subpath = (Path *) lfirst(lc);
			Bitmapset  *subserials;

			subserials = get_param_path_clause_serials(subpath);
			if (lc == list_head(apath->subpaths))
				pserials = bms_copy(subserials);
			else
				pserials = bms_int_members(pserials, subserials);
		}
		return pserials;
	}
	else
	{
		/*
		 * Otherwise, it's a baserel path and we can use the
		 * previously-computed set of serial numbers.
		 */
		return path->param_info->ppi_serials;
	}
}
