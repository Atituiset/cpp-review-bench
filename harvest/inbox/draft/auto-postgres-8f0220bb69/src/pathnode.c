// AUTO-DRAFT from postgres/postgres PR #f9bc96514afc9c71d533568f62dc613be8356c4d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
	 * to apply get_baserel_parampathinfo to construct a full ParamPathInfo
	 * for the path.  This supports building a Memoize path atop this path,
	 * and if this is a partitioned table the info may be useful for run-time
	 * pruning (cf make_partition_pruneinfo()).
	 *
	 * However, if we don't have "root" then that won't work and we fall back
	 * on the simpler get_appendrel_parampathinfo.  There's no point in doing
/* …（同文件无关代码省略）… */
	pathnode->path.pathtype = T_Material;
	pathnode->path.parent = rel;
	pathnode->path.pathtarget = rel->reltarget;
	pathnode->path.param_info = subpath->param_info;
	pathnode->path.parallel_aware = false;
	pathnode->path.parallel_safe = rel->consider_parallel &&
		subpath->parallel_safe;
/* …（同文件无关代码省略）… */
	pathnode->path.pathtype = T_Memoize;
	pathnode->path.parent = rel;
	pathnode->path.pathtarget = rel->reltarget;
	pathnode->path.param_info = subpath->param_info;
	pathnode->path.parallel_aware = false;
	pathnode->path.parallel_safe = rel->consider_parallel &&
		subpath->parallel_safe;
/* …（同文件无关代码省略）… */
	pathnode->path.pathtype = T_Result;
	pathnode->path.parent = rel;
	pathnode->path.pathtarget = target;
	pathnode->path.param_info = subpath->param_info;
	pathnode->path.parallel_aware = false;
	pathnode->path.parallel_safe = rel->consider_parallel &&
		subpath->parallel_safe &&
/* …（同文件无关代码省略）… */
	pathnode->path.parent = rel;
	/* Sort doesn't project, so use source path's pathtarget */
	pathnode->path.pathtarget = subpath->pathtarget;
	pathnode->path.param_info = subpath->param_info;
	pathnode->path.parallel_aware = false;
	pathnode->path.parallel_safe = rel->consider_parallel &&
		subpath->parallel_safe;
/* …（同文件无关代码省略）… */
	pathnode->path.parent = rel;
	/* Sort doesn't project, so use source path's pathtarget */
	pathnode->path.pathtarget = subpath->pathtarget;
	pathnode->path.param_info = subpath->param_info;
	pathnode->path.parallel_aware = false;
	pathnode->path.parallel_safe = rel->consider_parallel &&
		subpath->parallel_safe;
/* …（同文件无关代码省略）… */
	pathnode->path.parent = rel;
	/* Unique doesn't project, so use source path's pathtarget */
	pathnode->path.pathtarget = subpath->pathtarget;
	pathnode->path.param_info = subpath->param_info;
	pathnode->path.parallel_aware = false;
	pathnode->path.parallel_safe = rel->consider_parallel &&
		subpath->parallel_safe;
/* …（同文件无关代码省略）… */
	pathnode->path.pathtype = T_Agg;
	pathnode->path.parent = rel;
	pathnode->path.pathtarget = target;
	pathnode->path.param_info = subpath->param_info;
	pathnode->path.parallel_aware = false;
	pathnode->path.parallel_safe = rel->consider_parallel &&
		subpath->parallel_safe;
/* …（同文件无关代码省略）… */
	pathnode->path.pathtype = T_Agg;
	pathnode->path.parent = rel;
	pathnode->path.pathtarget = target;
	pathnode->path.param_info = subpath->param_info;
	pathnode->path.parallel_aware = false;
	pathnode->path.parallel_safe = rel->consider_parallel &&
		subpath->parallel_safe;
