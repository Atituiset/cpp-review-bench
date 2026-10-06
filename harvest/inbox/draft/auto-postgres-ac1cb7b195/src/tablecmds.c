// AUTO-DRAFT from postgres/postgres PR #80707c10456f3b29202899c62619b4a8d81fe8f9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

	Expr	   *partition_constraint;	/* for attach partition validation */
	/* true, if validating default due to some other attach/detach */
	bool		validate_default;
	/* Objects to rebuild after completing ALTER TYPE operations */
	List	   *changedConstraintOids;	/* OIDs of constraints to rebuild */
	List	   *changedConstraintDefs;	/* string definitions of same */
	List	   *changedIndexOids;	/* OIDs of indexes to rebuild */
