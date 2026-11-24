# auto-postgres-c1df9d80f6

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
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6a0a66b3e7ac394c499708b790effea9a564ef0d (https://github.com/postgres/postgres/commit/6a0a66b3e7ac394c499708b790effea9a564ef0d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1507；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6a0a66b3e7ac394c499708b790effea9a564ef0d Record logical replication conflicts in the conflict log table. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1495,22 +1495,26 @@ AlterSubscription_refresh_seq(Subscription *sub, char *conninfo)
 }
 
 /*
- * Common checks for altering failover, two_phase, and retain_dead_tuples
- * options.
+ * Common checks for altering failover, two_phase, retain_dead_tuples, and
+ * conflict_log_destination options.
  */
 static void
 CheckAlterSubOption(Subscription *sub, const char *option,
 					bool slot_needs_update, bool isTopLevel)
 {
 	Assert(strcmp(option, "failover") == 0 ||
 		   strcmp(option, "two_phase") == 0 ||
-		   strcmp(option, "retain_dead_tuples") == 0);
+		   strcmp(option, "retain_dead_tuples") == 0 ||
+		   strcmp(option, "conflict_log_destination") == 0);
 
 	/*
-	 * Altering the retain_dead_tuples option does not update the slot on the
-	 * publisher.
+	 * Only the failover and two_phase options change the slot on the
+	 * publisher. Altering retain_dead_tuples or conflict_log_destination does
+	 * not.
 	 */
-	Assert(!slot_needs_update || strcmp(option, "retain_dead_tuples") != 0);
+	if (slot_needs_update)
+		Assert(strcmp(option, "failover") == 0 ||
+			   strcmp(option, "two_phase") == 0);
 
 	/*
 	 * Do not allow changing the option if the subscription is enabled. This
@@ -1555,6 +1559,10 @@ CheckAlterSubOption(Subscription *sub, const char *option,
 	 * avoid the race conditions described above, but we maintain the
 	 * restriction for both enable and disable operations for the sake of
 	 * consistency.
+	 *
+	 * Also, when two_phase is enabled, do not allow changing the
+	 * conflict_log_destination option to 'log' when the subscription is
+	 * enabled; see AlterSubscription() and DropSubscription() for details.
 	 */
 	if (sub->enabled)
 		ereport(ERROR,
@@ -2110,6 +2118,57 @@ AlterSubscription(ParseState *pstate, AlterSubscriptionStmt *stmt,
 						bool		update_relid;
 						Oid			relid = InvalidOid;
 
+						/*
+						 * Similar to DropSubscription(), when two_phase is
+						 * enabled, changing to 'log' drops the conflict log
+						 * table, which can be blocked forever if a prepared
+						 * transaction of the subscription holds a lock on it.
+						 * So allow it only when the subscription is disabled,
+						 * no worker is running, and there are no prepared
+						 * transactions.
+						 *
+						 * XXX Another option would be to allow this on an
+						 * enabled subscription by restarting its workers. We
+						 * chose to require the subscription to be disabled
+						 * instead, following the precedent of existing
+						 * options such as two_phase and retain_dead_tuples
+						 * (see CheckAlterSubOption()).
+						 */
+						if (CONFLICTS_LOGGED_TO_TABLE(old_dest) &&
+							!CONFLICTS_LOGGED_TO_TABLE(opts.conflictlogdest) &&
+							sub->twophasestate != LOGICALREP_TWOPHASE_STATE_DISABLED)
+						{
+							CheckAlterSubOption(sub, "conflict_log_destination",
+												false, isTopLevel);
+
+							/*
+							 * Note that workers may still survive even if the
+							 * subscription has been disabled.
+							 *
+							 * Ensure workers have already exited to avoid
+							 * getting prepared transactions while we are
+							 * dropping the conflict log table.
+							 */
+							if (logicalrep_workers_find(subid, true, true))
+								ereport(ERROR,
+										(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
+										 errmsg("cannot alter option \"%s\" when logical replication worker is still running",
+												"conflict_log_destination"),
+										 errhint("Try again after some time.")));
+
+							/*
+							 * Disallow the operation when a prepared
+							 * transaction exists.
+							 */
+							if (LookupGXactBySubid(subid))
+								ereport(ERROR,
+										(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
+										 errmsg("cannot alter option \"%s\" when prepared transactions exist",
+												"conflict_log_destination"),
+										 errdetail("Dropping the conflict log table requires a lock that a prepared transaction of this subscription may be holding."),
+	
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`changeDependencyOnOwner`
- 外部函数：`ereport`
- 外部函数：`list_free`
- 外部函数：`strcmp`
- 外部函数：`superuser_arg`
- 大写宏：`ERROR`
- 大写宏：`GETSTRUCT`
- 大写宏：`NULL`
- 大写宏：`WARNING`
- 外部类型：`AccessExclusiveLock`
- 外部类型：`AclResult`
- 外部类型：`Altering`
- 外部类型：`Anum_pg_subscription_subconflictlogdest`
- 外部类型：`Common`
- 外部类型：`ConflictLogDestNames`
- 外部类型：`Datum`
- 外部类型：`Do`
- 外部类型：`Form_pg_subscription`
- 外部类型：`InvalidOid`
- 外部类型：`Oid`
- 外部类型：`Remove`
- 外部类型：`Subscription`
- 外部类型：`SubscriptionRelationId`
- 外部类型：`This`
- 外部类型：`Update`
- 外部类型：`Wake`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-c1df9d80f6` → 本草稿移入 `cases/defect/auto-postgres-c1df9d80f6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
