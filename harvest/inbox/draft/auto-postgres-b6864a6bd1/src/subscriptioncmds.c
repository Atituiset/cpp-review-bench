// AUTO-DRAFT from postgres/postgres PR #fc1900915b140aaed33fdcccdf85eec1a0035755
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define SUBOPT_FAILOVER				0x00002000
/* …（同文件无关代码省略）… */
#define IsSet(val, bits)  (((val) & (bits)) == (bits))
/* …（同文件无关代码省略）… */
								 errmsg("cannot disable \"two_phase\" when prepared transactions exist"),
								 errhint("Resolve these transactions and try again.")));

					/* Change system catalog accordingly */
					values[Anum_pg_subscription_subtwophasestate - 1] =
						CharGetDatum(opts.twophase ?
									 LOGICALREP_TWOPHASE_STATE_PENDING :
									 LOGICALREP_TWOPHASE_STATE_DISABLED);
					replaces[Anum_pg_subscription_subtwophasestate - 1] = true;
				}

				if (IsSet(opts.specified_opts, SUBOPT_FAILOVER))
