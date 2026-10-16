// AUTO-DRAFT from postgres/postgres PR #a39e53a700c5b728f2f7dab964e33b157979fb3a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
		 * Permissions checks relax the pg_terminate_backend checks in two
		 * ways, both by omitting the !OidIsValid(proc->roleId) check:
		 *
		 * - Accept terminating autovacuum workers, since DROP DATABASE
		 * without FORCE terminates them.
		 *
		 * - Accept terminating bgworkers.  For bgworker authors, it's
		 * convenient to be able to recommend FORCE if a worker is blocking
/* …（同文件无关代码省略）… */

			if (proc != NULL)
			{
				if (superuser_arg(proc->roleId) && !superuser())
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied to terminate process"),
							 errdetail("Only roles with the %s attribute may terminate processes of roles with the %s attribute.",
									   "SUPERUSER", "SUPERUSER")));

				if (!has_privs_of_role(GetUserId(), proc->roleId) &&
					!has_privs_of_role(GetUserId(), ROLE_PG_SIGNAL_BACKEND))
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
