// AUTO-DRAFT from torvalds/linux PR #eef2bc5a82c25d25c3082f5d86d97a300caeb678
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <net/af_unix.h>

enum landlock_request_type;
struct dentry;
struct landlock_blockers;
struct landlock_domain;
/* …（同文件无关代码省略）… */
struct path;
struct sock;
struct task_struct;

static_assert(sizeof(access_mask_t) <= sizeof(u64));

TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_FS_CHANGE_TOPOLOGY);
TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_FS_ACCESS);
TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_NET_ACCESS);
TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_NAMESPACE);
TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_CAPABILITY);

#ifdef CREATE_TRACE_POINTS
/* …（同文件无关代码省略）… */
 * @namespace_id: Namespace ID, not directly visible from user space when
 *                creation was denied.
 *
 * Emitted when a Landlock domain denies namespace use.
 */
TRACE_EVENT(landlock_deny_permission_namespace,

/* …（同文件无关代码省略）… */
		__entry->namespace_id	= namespace_id;
	),

	TP_printk("domain=%llx same_exec=%d logged=%d blockers=%s namespace_type=0x%x namespace_id=%llu",
		__entry->domain_id, __entry->same_exec, __entry->logged,
		__entry->blockers_type == LANDLOCK_REQUEST_NAMESPACE ?
			__print_flags(__entry->blockers_access, "|",
				      _LANDLOCK_PERMISSION_BLOCKER_NAMES) :
			"unknown",
		__entry->namespace_type, __entry->namespace_id)
);

/**
/* …（同文件无关代码省略）… */
 * @logged: Whether this denial was selected for audit logging.
 * @blockers: Request type and final missing permission subset (never NULL).
 * @capability: CAP_* number that was denied.
 *
 * Emitted when a Landlock domain denies capability use, except for checks
 * made with CAP_OPT_NOAUDIT, which are denied without emitting this event.
/* …（同文件无关代码省略）… */

	TP_PROTO(const struct landlock_hierarchy *hierarchy, bool same_exec,
		 bool logged, const struct landlock_blockers *blockers,
		 int capability),

	TP_ARGS(hierarchy, same_exec, logged, blockers, capability),

	TP_STRUCT__entry(
		__field(	u64,		domain_id	)
