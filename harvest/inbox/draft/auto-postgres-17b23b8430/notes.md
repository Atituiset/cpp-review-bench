# auto-postgres-17b23b8430

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#e5f6a7a28c4ab82a80fd0fa89d9f77b80ae97c07](https://github.com/postgres/postgres/commit/e5f6a7a28c4ab82a80fd0fa89d9f77b80ae97c07) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 116 |
| 编译错误数（gcc syntax-only） | 37（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e5f6a7a28c4ab82a80fd0fa89d9f77b80ae97c07 (https://github.com/postgres/postgres/commit/e5f6a7a28c4ab82a80fd0fa89d9f77b80ae97c07)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2723；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e5f6a7a28c4ab82a80fd0fa89d9f77b80ae97c07 Don't free a cached RI plan that may be in use :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -231,6 +231,26 @@ static dclist_head ri_constraint_cache_valid_list;
  */
 static FastPathMeta *ri_fpmeta_dead_list = NULL;
 
+/*
+ * Stack of the plans being executed by ri_PerformCheck() calls currently
+ * running, i.e. RI queries whose SPI_execute_snapshot() has not returned.
+ * Innermost of those appears last in the array.  A plan that
+ * ri_FetchPreparedPlan() finds invalid cannot be freed while it is on this
+ * stack.  It instead goes on ri_plan_dead_list and is freed by
+ * ri_FreeDeadPlans() once it is no longer on the stack, which is checked
+ * whenever a check finishes normally, or by AtEOXact_RI() at the latest.
+ * Both live in TopMemoryContext.
+ *
+ * ri_PerformCheck() pops its plan whether the query returns or an error
+ * is thrown, by restoring the depth it saw at push time, so the stack is
+ * exact at all times.  A plan on the stack is never freed until the stack
+ * is cut back past it, so no entry can dangle.
+ */
+static SPIPlanPtr *ri_executing_plans = NULL;
+static int	ri_executing_depth = 0;
+static int	ri_executing_maxdepth = 0;
+static List *ri_plan_dead_list = NIL;
+
 /*
  * Local function prototypes
  */
@@ -262,6 +282,9 @@ static void InvalidateConstraintCacheCallBack(Datum arg, SysCacheIdentifier cach
 											  uint32 hashvalue);
 static SPIPlanPtr ri_FetchPreparedPlan(RI_QueryKey *key);
 static void ri_HashPreparedPlan(RI_QueryKey *key, SPIPlanPtr plan);
+static int	ri_PushExecutingPlan(SPIPlanPtr plan);
+static bool ri_PlanIsExecuting(SPIPlanPtr plan);
+static void ri_FreeDeadPlans(void);
 static RI_CompareHashEntry *ri_HashCompareOp(Oid eq_opr, Oid typeid);
 
 static void ri_CheckTrigger(FunctionCallInfo fcinfo, const char *funcname,
@@ -2628,6 +2651,7 @@ ri_PerformCheck(const RI_ConstraintInfo *riinfo,
 	Snapshot	crosscheck_snapshot;
 	int			limit;
 	int			spi_result;
+	int			exec_depth;
 	Oid			save_userid;
 	int			save_sec_context;
 	Datum		vals[RI_MAX_NUMKEYS * 2];
@@ -2719,15 +2743,42 @@ ri_PerformCheck(const RI_ConstraintInfo *riinfo,
 	 * Set fire_triggers to false to ensure that AFTER triggers are queued in
 	 * the outer query's after-trigger context and fire after all RI updates
 	 * on the same row are complete, rather than immediately.
+	 *
+	 * The plan is on the stack of executing plans while the query runs, so
+	 * that a nested check finding it invalid does not free it; see
+	 * ri_FetchPreparedPlan().  Pop it even if the query throws an error, so
+	 * that, whatever catches the error, the stack holds only the plans still
+	 * executing.  We don't free anything in the finally block, so if the
+	 * query throws an error, whatever was queued while it ran stays queued.
+	 * The next check that finishes normally frees it, once no check is
+	 * executing it, or at the latest AtEOXact_RI() does.
 	 */
-	spi_result = SPI_execute_snapshot(qplan,
-									  vals, nulls,
-									  test_snapshot, crosscheck_snapshot,
-									  false, false, limit);
+	exec_depth = ri_PushExecutingPlan(qplan);
+	PG_TRY();
+	{
+		spi_result = SPI_execute_snapshot(qplan,
+										  vals, nulls,
+										  test_snapshot, crosscheck_snapshot,
+										  false, false, limit);
+	}
+	PG_FINALLY();
+	{
+		/* Pop qplan, whether the query returned normally or threw an error. */
+		ri_executing_depth = exec_depth;
+	}
+	PG_END_TRY();
 
 	/* Restore UID and security context */
 	SetUserIdAndSecContext(save_userid, save_sec_context);
 
+	/*
+	 * Free the queued plans that no check is executing any more, which may
+	 * include qplan itself now that it is popped.  Checks further up the
+	 * stack may still be executing others, which stay queued.
+	 */
+	if (ri_plan_dead_list != NIL)
+		ri_FreeDeadPlans();
+
 	/* Check result */
 	if (spi_result < 0)
 		elog(ERROR, "SPI_execute_snapshot returned %s", SPI_result_code_string(spi_result));
@@ -3689,12 +3740,32 @@ ri_FetchPreparedPlan(RI_QueryKey *key)
 		return plan;
 
 	/*
-	 * Otherwise we might as well flush the cached plan now, to free a little
-	 *
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`anyrange`
- 外部函数：`dclist_container`
- 外部函数：`dclist_count`
- 外部函数：`dclist_delete_from`
- 外部函数：`elog`
- 外部函数：`entry`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`find_coercion_pathway`
- 外部函数：`fmgr_info_cxt`
- 外部函数：`format_type_be`
- 外部函数：`get_opcode`
- 外部函数：`hash_create`
- 外部函数：`hash_search`
- 外部函数：`op_input_types`
- 外部函数：`operators`
- 外部函数：`pfree`
- 外部函数：`range_agg`
- 外部函数：`ri_CompareWithCast`
- 外部函数：`ri_FastPathCheck`
- 大写宏：`AFTER`
- 大写宏：`ALTER`
- 大写宏：`AMOPOPID`
- 大写宏：`ANYARRAY`
- 大写宏：`ANYENUM`
- 大写宏：`CALLED_AS_TRIGGER`
- 大写宏：`CASTSOURCETARGET`
- 大写宏：`CAUTION`
- 大写宏：`COERCION_IMPLICIT`
- 大写宏：`COERCION_PATH_FUNC`
- 大写宏：`COERCION_PATH_RELABELTYPE`
- 大写宏：`CONSTROID`
- 大写宏：`DDL`
- 大写宏：`DELETE`
- 大写宏：`ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED`
- 大写宏：`ERROR`
- 大写宏：`HASHCTL`
- 大写宏：`HASH_BLOBS`
- 大写宏：`HASH_ELEM`
- 大写宏：`HASH_ENTER`
- 大写宏：`HASH_FIND`
- 大写宏：`INDEX_MAX_KEYS`
- 大写宏：`INSERT`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`PERIOD`
- 大写宏：`RECORD`
- 大写宏：`RI_FASTPATH_UNKNOWN`
- 大写宏：`RI_FASTPATH_UNUSABLE`
- 大写宏：`RI_FASTPATH_USABLE`
- 大写宏：`RI_PLAN_XXX`
- 大写宏：`ROW`
- 大写宏：`SET`
- 大写宏：`TRIGGER_FIRED_AFTER`
- 大写宏：`TRIGGER_FIRED_BY_DELETE`
- 大写宏：`TRIGGER_FIRED_BY_INSERT`
- 大写宏：`TRIGGER_FIRED_BY_UPDATE`
- 大写宏：`TRIGGER_FIRED_FOR_ROW`
- 大写宏：`UID`
- 大写宏：`UPDATE`
- 大写宏：`XXX`
- 外部类型：`Add`
- 外部类型：`Arrange`
- 外部类型：`At`
- 外部类型：`AtEOXact_`
- 外部类型：`AtEOXact_RI`
- 外部类型：`Being`
- 外部类型：`Check`
- 外部类型：`CoerceViaIO`
- 外部类型：`CoercionPathType`
- 外部类型：`Currently`
- 外部类型：`Datum`
- 外部类型：`Detach`
- 外部类型：`End`
- 外部类型：`FastPathMeta`
- 外部类型：`Find`
- 外部类型：`FmgrInfo`
- 外部类型：`Freeing`
- 外部类型：`FunctionCallInfo`
- 外部类型：`If`
- 外部类型：`InvalidOid`
- 外部类型：`InvalidateConstraintCacheCallBack`
- 外部类型：`It`
- 外部类型：`Likewise`
- 外部类型：`Link`
- 外部类型：`Local`
- 外部类型：`Lookup`
- 外部类型：`NameData`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`Nothing`
- 外部类型：`Oid`
- 外部类型：`On`
- 外部类型：`Otherwise`
- 外部类型：`Queue`
- 外部类型：`RI_CompareHashEntry`
- 外部类型：`RI_CompareKey`
- 外部类型：`RI_ConstraintInfo`
- 外部类型：`RI_QueryHashEntry`
- 外部类型：`RI_QueryKey`
- 外部类型：`Remove`
- 外部类型：`Restore`
- 外部类型：`SPIPlanPtr`
- 外部类型：`SPI_execute_snapshot`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`Snapshot`
- 外部类型：`SysCacheIdentifier`
- 外部类型：`TABLEs`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`TopMemoryContext`
- 外部类型：`TriggerData`
- 外部类型：`We`
- 外部类型：`padding`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-17b23b8430` → 本草稿移入 `cases/defect/auto-postgres-17b23b8430/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
