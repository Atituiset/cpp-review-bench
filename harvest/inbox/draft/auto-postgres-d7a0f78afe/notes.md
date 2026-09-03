# auto-postgres-d7a0f78afe

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#ad4b80b6f49bcac519175fcb632aa94cdb7f6948](https://github.com/postgres/postgres/commit/ad4b80b6f49bcac519175fcb632aa94cdb7f6948) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 150 |
| 编译错误数（gcc syntax-only） | 39（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ad4b80b6f49bcac519175fcb632aa94cdb7f6948 (https://github.com/postgres/postgres/commit/ad4b80b6f49bcac519175fcb632aa94cdb7f6948)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 3274；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ad4b80b6f49bcac519175fcb632aa94cdb7f6948 Remove quals made redundant by reducing outer joins to antijoins :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -146,7 +146,6 @@ static void distribute_qual_to_rels(PlannerInfo *root, Node *clause,
 									bool has_clone,
 									bool is_clone,
 									List **postponed_oj_qual_list);
-static bool check_redundant_nullability_qual(PlannerInfo *root, Node *clause);
 static Relids get_join_domain_min_rels(PlannerInfo *root, Relids domain_relids);
 static void check_mergejoinable(RestrictInfo *restrictinfo);
 static void check_hashjoinable(RestrictInfo *restrictinfo);
@@ -2871,10 +2870,6 @@ distribute_quals_to_rels(PlannerInfo *root, List *clauses,
  * 'qualscope' identifies what level of JOIN the qual came from syntactically.
  * 'ojscope' is needed if we decide to force the qual up to the outer-join
  * level, which will be ojscope not necessarily qualscope.
- *
- * At the time this is called, root->join_info_list must contain entries for
- * at least those special joins that are syntactically below this qual.
- * (We now need that only for detection of redundant IS NULL quals.)
  */
 static void
 distribute_qual_to_rels(PlannerInfo *root, Node *clause,
@@ -3078,15 +3073,6 @@ distribute_qual_to_rels(PlannerInfo *root, Node *clause,
 		 */
 		is_pushed_down = true;
 
-		/*
-		 * It's possible that this is an IS NULL clause that's redundant with
-		 * a lower antijoin; if so we can just discard it.  We need not test
-		 * in any of the other cases, because this will only be possible for
-		 * pushed-down clauses.
-		 */
-		if (check_redundant_nullability_qual(root, clause))
-			return;
-
 		/* Feed qual to the equivalence machinery, if allowed by caller */
 		maybe_equivalence = allow_equivalence;
 
@@ -3253,53 +3239,6 @@ distribute_qual_to_rels(PlannerInfo *root, Node *clause,
 	distribute_restrictinfo_to_rels(root, restrictinfo);
 }
 
-/*
- * check_redundant_nullability_qual
- *	  Check to see if the qual is an IS NULL qual that is redundant with
- *	  a lower JOIN_ANTI join.
- *
- * We want to suppress redundant IS NULL quals, not so much to save cycles
- * as to avoid generating bogus selectivity estimates for them.  So if
- * redundancy is detected here, distribute_qual_to_rels() just throws away
- * the qual.
- */
-static bool
-check_redundant_nullability_qual(PlannerInfo *root, Node *clause)
-{
-	Var		   *forced_null_var;
-	ListCell   *lc;
-
-	/* Check for IS NULL, and identify the Var forced to NULL */
-	forced_null_var = find_forced_null_var(clause);
-	if (forced_null_var == NULL)
-		return false;
-
-	/*
-	 * If the Var comes from the nullable side of a lower antijoin, the IS
-	 * NULL condition is necessarily true.  If it's not nulled by anything,
-	 * there is no point in searching the join_info_list.  Otherwise, we need
-	 * to find out whether the nulling rel is an antijoin.
-	 */
-	if (forced_null_var->varnullingrels == NULL)
-		return false;
-
-	foreach(lc, root->join_info_list)
-	{
-		SpecialJoinInfo *sjinfo = (SpecialJoinInfo *) lfirst(lc);
-
-		/*
-		 * This test will not succeed if sjinfo->ojrelid is zero, which is
-		 * possible for an antijoin that was converted from a semijoin; but in
-		 * such a case the Var couldn't have come from its nullable side.
-		 */
-		if (sjinfo->jointype == JOIN_ANTI && sjinfo->ojrelid != 0 &&
-			bms_is_member(sjinfo->ojrelid, forced_null_var->varnullingrels))
-			return true;
-	}
-
-	return false;
-}
-
 /*
  * add_base_clause_to_rel
  *		Add 'restrictinfo' as a baserestrictinfo to the base relation denoted
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`add_join_clause_to_rels`
- 外部函数：`bms_add_members`
- 外部函数：`bms_copy`
- 外部函数：`bms_del_member`
- 外部函数：`bms_del_members`
- 外部函数：`bms_equal`
- 外部函数：`bms_get_singleton_member`
- 外部函数：`bms_intersect`
- 外部函数：`bms_is_empty`
- 外部函数：`bms_is_member`
- 外部函数：`bms_is_subset`
- 外部函数：`bms_membership`
- 外部函数：`bms_overlap`
- 外部函数：`check_redundant_nullability_qual`
- 外部函数：`contain_volatile_functions`
- 外部函数：`copyObject`
- 外部函数：`elog`
- 外部函数：`equivalence`
- 外部函数：`exprType`
- 外部函数：`expr_is_nonnullable`
- 外部函数：`find_base_rel`
- 外部函数：`find_forced_null_var`
- 外部函数：`find_placeholder_info`
- 外部函数：`get_mergejoin_opfamilies`
- 外部函数：`initialize_mergeclause_eclasses`
- 外部函数：`is`
- 外部函数：`is_opclause`
- 外部函数：`is_orclause`
- 外部函数：`join`
- 外部函数：`lappend`
- 外部函数：`lfirst`
- 外部函数：`linitial`
- 外部函数：`list_free`
- 外部函数：`list_length`
- 外部函数：`lookup_type_cache`
- 外部函数：`lsecond`
- 外部函数：`makeBoolConst`
- 外部函数：`makeNode`
- 外部函数：`make_restrictinfo`
- 外部函数：`nodeTag`
- 外部函数：`op_hashjoinable`
- 外部函数：`op_mergejoinable`
- 外部函数：`process_equivalence`
- 外部函数：`pull_var_clause`
- 外部函数：`pull_varnos`
- 外部函数：`restriction_is_or_clause`
- 大写宏：`API`
- 大写宏：`BMS_MULTIPLE`
- 大写宏：`BMS_SINGLETON`
- 大写宏：`ERROR`
- 大写宏：`FALSE`
- 大写宏：`FULL`
- 大写宏：`INNER`
- 大写宏：`IS_NOT_NULL`
- 大写宏：`IS_NULL`
- 大写宏：`JOIN`
- 大写宏：`JOIN_ANTI`
- 大写宏：`JOIN_FULL`
- 大写宏：`JOIN_LEFT`
- 大写宏：`LATERAL`
- 大写宏：`LHS`
- 大写宏：`NIL`
- 大写宏：`NOT`
- 大写宏：`NOTNULL_SOURCE_RELOPT`
- 大写宏：`NULL`
- 大写宏：`OUTER`
- 大写宏：`PVC_INCLUDE_PLACEHOLDERS`
- 大写宏：`PVC_RECURSE_AGGREGATES`
- 大写宏：`PVC_RECURSE_WINDOWFUNCS`
- 大写宏：`RELKIND_PARTITIONED_TABLE`
- 大写宏：`RINFO_IS_PUSHED_DOWN`
- 大写宏：`SQL`
- 大写宏：`TRUE`
- 大写宏：`TYPECACHE_EQ_OPR`
- 大写宏：`TYPECACHE_HASH_PROC`
- 大写宏：`WHERE`
- 外部类型：`Add`
- 外部类型：`Although`
- 外部类型：`Assert`
- 外部类型：`At`
- 外部类型：`BoolExpr`
- 外部类型：`Build`
- 外部类型：`But`
- 外部类型：`Check`
- 外部类型：`Currently`
- 外部类型：`Degenerate`
- 外部类型：`Don`
- 外部类型：`Either`
- 外部类型：`Empty`
- 外部类型：`EquivalenceClass`
- 外部类型：`EquivalenceClasses`
- 外部类型：`Expr`
- 外部类型：`Feed`
- 外部类型：`Fields`
- 外部类型：`For`
- 外部类型：`However`
- 外部类型：`Ideally`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Index`
- 外部类型：`It`
- 外部类型：`JoinDomain`
- 外部类型：`JoinTreeItem`
- 外部类型：`Likewise`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Lookup`
- 外部类型：`Memoize`
- 外部类型：`No`
- 外部类型：`Node`
- 外部类型：`Non`
- 外部类型：`Nope`
- 外部类型：`Normal`
- 外部类型：`Normally`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`NullTest`
- 外部类型：`Oid`
- 外部类型：`OpExpr`
- 外部类型：`Otherwise`
- 外部类型：`OuterJoinClauseInfo`
- 外部类型：`PlaceHolderInfo`
- 外部类型：`PlaceHolderVar`
- 外部类型：`PlannerInfo`
- 外部类型：`Pseudoconstant`
- 外部类型：`RangeTblEntries`
- 外部类型：`RangeTblEntry`
- 外部类型：`RelOptInfo`
- 外部类型：`Relids`
- 外部类型：`RestrictInfo`
- 外部类型：`RestrictInfos`
- 外部类型：`Result`
- 外部类型：`Retrieve`
- 外部类型：`Shouldn`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`SpecialJoinInfo`
- 外部类型：`Specifically`
- 外部类型：`Substitute`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`Top`
- 外部类型：`TypeCacheEntry`
- 外部类型：`Update`
- 外部类型：`Var`
- 外部类型：`Variable`
- 外部类型：`Vars`
- 外部类型：`We`
- 外部类型：`When`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-d7a0f78afe` → 本草稿移入 `cases/defect/auto-postgres-d7a0f78afe/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
