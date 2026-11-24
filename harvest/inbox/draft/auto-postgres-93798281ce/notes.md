# auto-postgres-93798281ce

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#6a0a66b3e7ac394c499708b790effea9a564ef0d](https://github.com/postgres/postgres/commit/6a0a66b3e7ac394c499708b790effea9a564ef0d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 108 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6a0a66b3e7ac394c499708b790effea9a564ef0d (https://github.com/postgres/postgres/commit/6a0a66b3e7ac394c499708b790effea9a564ef0d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 792；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6a0a66b3e7ac394c499708b790effea9a564ef0d Record logical replication conflicts in the conflict log table. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -16,17 +16,24 @@
 
 #include "access/commit_ts.h"
 #include "access/genam.h"
+#include "access/heapam.h"
 #include "access/tableam.h"
 #include "catalog/heap.h"
 #include "catalog/pg_am.h"
 #include "catalog/pg_namespace.h"
 #include "catalog/toasting.h"
 #include "executor/executor.h"
+#include "funcapi.h"
 #include "pgstat.h"
 #include "replication/conflict.h"
 #include "replication/worker_internal.h"
 #include "storage/lmgr.h"
+#include "utils/array.h"
+#include "utils/builtins.h"
+#include "utils/json.h"
 #include "utils/lsyscache.h"
+#include "utils/pg_lsn.h"
+#include "utils/xid8.h"
 
 /*
  * String representations for the supported conflict logging destinations.
@@ -56,13 +63,18 @@ typedef struct ConflictLogColumnDef
  * type OID; the table is created in this column order by
  * create_conflict_log_table().
  *
- * The tuple/key columns (replica_identity, remote_tuple, local_conflicts) are
- * typed json rather than jsonb on purpose: they hold an exact audit snapshot
- * of the applied tuples and replica identity, and json preserves the verbatim
+ * The replica_identity and local_conflicts columns are typed json rather than
+ * jsonb on purpose: they hold an exact audit snapshot of the replica identity
+ * key values and local conflict metadata, and json preserves the verbatim
  * representation whereas jsonb would normalize it. Indexing them (jsonb's main
  * advantage) wouldn't help anyway, as the conflict log is looked up by its
  * scalar columns (relid, conflict_type, commit timestamp) while these json
  * columns are per-conflict payload to inspect, not search keys.
+ *
+ * 'local_conflicts' is typed as an array of JSON objects (json[]), not a
+ * single json object, so that a future conflict type needing to record
+ * multiple local rows for one remote operation doesn't require a
+ * backward-incompatible schema change.
  */
 static const ConflictLogColumnDef ConflictLogSchema[] = {
 	{.attname = "relid", .atttypid = OIDOID},
@@ -72,15 +84,41 @@ static const ConflictLogColumnDef ConflictLogSchema[] = {
 	{.attname = "remote_xid", .atttypid = XIDOID},
 	{.attname = "remote_commit_lsn", .atttypid = LSNOID},
 	{.attname = "remote_commit_ts", .atttypid = TIMESTAMPTZOID},
-	{.attname = "remote_origin", .atttypid = TEXTOID},
+	{.attname = "apply_origin", .atttypid = TEXTOID},
 	{.attname = "replica_identity_full", .atttypid = BOOLOID},
 	{.attname = "replica_identity", .atttypid = JSONOID},
-	{.attname = "remote_tuple", .atttypid = JSONOID},
-	{.attname = "local_conflicts", .atttypid = JSONARRAYOID}
+	{.attname = "local_conflicts", .atttypid = JSONARRAYOID},
+	{.attname = "has_omitted_values", .atttypid = BOOLOID}
 };
 
 #define NUM_CONFLICT_ATTRS ((AttrNumber) lengthof(ConflictLogSchema))
 
+/*
+ * Maximum size in bytes of a replica identity key value recorded in the
+ * conflict log table. A larger value is replaced by a marker holding its
+ * length. See build_index_key_json().
+ *
+ * The cap applies to the length of the type's text output before JSON escaping.
+ * Since escape_json() expands each byte at most sixfold (to \uXXXX), and an
+ * index has at most INDEX_MAX_KEYS (32) key columns, the worst-case JSON output
+ * is 64kB * 6 * 32 = 12MB, comfortably inside the 1GB limit on a json value.
+ * Ordinary keys are orders of magnitude below the cap, so in practice nothing
+ * is ever omitted.
+ */
+#define CONFLICT_MAX_VALUE_SIZE (64 * 1024)
+
+/*
+ * Schema for the elements within the 'local_conflicts' JSON array.
+ */
+static const ConflictLogColumnDef LocalConflictSchema[] =
+{
+	{.attname = "xid", .atttypid = XID8OID},
+	{.attname = "commit_ts", .atttypid = TIMESTAMPTZOID},
+	{.attname = "origin", .atttypid = TEXTOID}
+};
+
+#define NUM_LOCAL_CONFLICT_ATTRS lengthof(LocalConflictSchema)
+
 static const char *const ConflictTypeNames[] = {
 	[CT_INSERT_EXISTS] = "insert_exists",
 	[CT_UPDATE_ORIGIN_DIFFERS] = "update_origin_differs",
@@ -92,6 +130,8 @@ static const char *const ConflictTypeNames[] = {
 	[CT_M
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_`
- 外部函数：`appendStringInfo`
- 外部函数：`appendStringInfoChar`
- 外部函数：`appendStringInfoString`
- 外部函数：`bms_union`
- 外部函数：`columns`
- 外部函数：`create_conflict_log_table`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errdetail_internal`
- 外部函数：`get_rel_name`
- 外部函数：`index_close`
- 外部函数：`index_open`
- 外部函数：`initStringInfo`
- 外部函数：`lengthof`
- 外部函数：`list_make1`
- 外部函数：`list_make2`
- 外部函数：`list_make3`
- 外部函数：`pgstat_report_subscription_conflict`
- 外部函数：`psprintf`
- 外部函数：`replorigin_by_oid`
- 外部函数：`resetStringInfo`
- 外部函数：`table_slot_create`
- 外部函数：`them`
- 外部函数：`timestamptz_to_str`
- 外部函数：`used`
- 大写宏：`BOOLOID`
- 大写宏：`CT_DELETE_MISSING`
- 大写宏：`CT_DELETE_ORIGIN_DIFFERS`
- 大写宏：`CT_INSERT_EXISTS`
- 大写宏：`CT_MULTIPLE_UNIQUE_CONFLICTS`
- 大写宏：`CT_UPDATE_DELETED`
- 大写宏：`CT_UPDATE_EXISTS`
- 大写宏：`CT_UPDATE_MISSING`
- 大写宏：`CT_UPDATE_ORIGIN_DIFFERS`
- 大写宏：`ERRCODE_T_R_SERIALIZATION_FAILURE`
- 大写宏：`ERRCODE_UNIQUE_VIOLATION`
- 大写宏：`FULL`
- 大写宏：`IDENTITY`
- 大写宏：`INDEX_MAX_KEYS`
- 大写宏：`JSONARRAYOID`
- 大写宏：`JSONOID`
- 大写宏：`LSNOID`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`OIDOID`
- 大写宏：`REPLICA`
- 大写宏：`SQL`
- 大写宏：`SQLSTATE`
- 大写宏：`TEXTOID`
- 大写宏：`TIMESTAMPTZOID`
- 大写宏：`TTS_IS_VIRTUAL`
- 大写宏：`XIDOID`
- 大写宏：`XXX`
- 外部类型：`Add`
- 外部类型：`Although`
- 外部类型：`AttrNumber`
- 外部类型：`Bitmapset`
- 外部类型：`BuildIndexValueDescription`
- 外部类型：`Builds`
- 外部类型：`Column`
- 外部类型：`ConflictLogColumnDef`
- 外部类型：`ConflictLogSchema`
- 外部类型：`ConflictTupleInfo`
- 外部类型：`ConflictType`
- 外部类型：`ConflictTypeNames`
- 外部类型：`Construct`
- 外部类型：`Could`
- 外部类型：`Data`
- 外部类型：`Datum`
- 外部类型：`Deleting`
- 外部类型：`EState`
- 外部类型：`ExecBuildSlotValueDescription`
- 外部类型：`Form`
- 外部类型：`FormIndexDatum`
- 外部类型：`Get`
- 外部类型：`Helper`
- 外部类型：`If`
- 外部类型：`Indexing`
- 外部类型：`Initialize`
- 外部类型：`Insert`
- 外部类型：`InvalidReplOriginId`
- 外部类型：`IsIndexUsableForReplicaIdentityFull`
- 外部类型：`It`
- 外部类型：`Key`
- 外部类型：`List`
- 外部类型：`MySubscription`
- 外部类型：`NoLock`
- 外部类型：`Note`
- 外部类型：`Oid`
- 外部类型：`Otherwise`
- 外部类型：`Relation`
- 外部类型：`ReplOriginId`
- 外部类型：`Report`
- 外部类型：`ResultRelInfo`
- 外部类型：`RowExclusiveLock`
- 外部类型：`See`
- 外部类型：`Skip`
- 外部类型：`String`
- 外部类型：`StringInfo`
- 外部类型：`StringInfoData`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`TimestampTz`
- 外部类型：`TransactionId`
- 外部类型：`TupleDesc`
- 外部类型：`TupleTableSlot`
- 外部类型：`Updating`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-93798281ce` → 本草稿移入 `cases/defect/auto-postgres-93798281ce/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
