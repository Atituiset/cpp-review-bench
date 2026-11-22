// AUTO-DRAFT from torvalds/linux PR #eef2bc5a82c25d25c3082f5d86d97a300caeb678
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <time.h>

 * enacted.  Vice versa, access rights that are not specifically listed here are
 * not going to be denied by this ruleset when it is enacted.
 *
 * For historical reasons, the %LANDLOCK_ACCESS_FS_REFER right is always denied
 * by default, even when its bit is not set in @handled_access_fs.  In order to
 * add new rules with this access right, the bit must still be set explicitly
 * (cf. `Filesystem flags`_).
 *
 * The explicit listing of *handled access rights* is required for backwards
 * compatibility reasons.  In most use cases, processes that use Landlock will
/* …（同文件无关代码省略）… */
 *   This access right is available since the second version of the Landlock
 *   ABI.
 *
 *   This is the only access right which is denied by default by any ruleset,
 *   even if the right is not specified as handled at ruleset creation time.
 *   The only way to make a ruleset grant this right is to explicitly allow it
 *   for a specific directory by adding a matching rule to the ruleset.
 *
 *   In particular, when using the first Landlock ABI version, Landlock will
 *   always deny attempts to reparent files between different directories.
/* …（同文件无关代码省略）… */
 * - %LANDLOCK_PERMISSION_CAPABILITY_USE: Restrict the use of specific Linux
 *   capabilities.  A process in a Landlock domain that handles this permission
 *   is denied from exercising capabilities that are not explicitly allowed by a
 *   %LANDLOCK_RULE_CAPABILITY rule.  This hook is purely restrictive: it can
 *   deny capabilities that the kernel would otherwise grant, but it can never
 *   grant capabilities that the kernel already denied.  Support added in
 *   Landlock ABI version 12.
 */
#define LANDLOCK_PERMISSION_NAMESPACE_USE			(1ULL << 0)
#define LANDLOCK_PERMISSION_CAPABILITY_USE			(1ULL << 1)
