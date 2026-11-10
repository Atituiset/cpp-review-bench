// AUTO-DRAFT from postgres/postgres PR #6a3da13d376f5950fd105eedddc4a1e829e6b398
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

	F_JSONB_IN, F_JSONB_OUT},
	{"oid", OIDOID, 0, 4, true, TYPALIGN_INT, TYPSTORAGE_PLAIN, InvalidOid,
	F_OIDIN, F_OIDOUT},
	{"aclitem", ACLITEMOID, 0, 16, false, TYPALIGN_DOUBLE, TYPSTORAGE_PLAIN, InvalidOid,
	F_ACLITEMIN, F_ACLITEMOUT},
	{"pg_node_tree", PG_NODE_TREEOID, 0, -1, false, TYPALIGN_INT, TYPSTORAGE_EXTENDED, DEFAULT_COLLATION_OID,
