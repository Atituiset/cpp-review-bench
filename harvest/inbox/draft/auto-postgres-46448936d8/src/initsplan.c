// AUTO-DRAFT from postgres/postgres PR #77e5aeef59cd2fa680e54e552767e56d07cbab4a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
				PlaceHolderInfo *phinfo = find_placeholder_info(root, phv);

				found_laterals = true;
				lateral_relids = bms_add_members(lateral_relids,
												 phinfo->ph_eval_at);
			}
			else
				Assert(false);
