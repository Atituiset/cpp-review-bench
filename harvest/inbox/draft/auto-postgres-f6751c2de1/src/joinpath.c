// AUTO-DRAFT from postgres/postgres PR #f9bc96514afc9c71d533568f62dc613be8356c4d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	 */
	if (extra->inner_unique)
	{
		Bitmapset  *ppi_serials;

		if (inner_path->param_info == NULL)
			return NULL;

		ppi_serials = inner_path->param_info->ppi_serials;

		foreach_node(RestrictInfo, rinfo, extra->restrictlist)
		{
			if (!bms_is_member(rinfo->rinfo_serial, ppi_serials))
				return NULL;
		}
	}
