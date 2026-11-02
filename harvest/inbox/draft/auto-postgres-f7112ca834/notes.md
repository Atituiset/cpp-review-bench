# auto-postgres-f7112ca834

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#f8da64e6ddfd5f33f1d70b3ae6f8561f1de09368](https://github.com/postgres/postgres/commit/f8da64e6ddfd5f33f1d70b3ae6f8561f1de09368) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 30 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f8da64e6ddfd5f33f1d70b3ae6f8561f1de09368 (https://github.com/postgres/postgres/commit/f8da64e6ddfd5f33f1d70b3ae6f8561f1de09368)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f8da64e6ddfd5f33f1d70b3ae6f8561f1de09368 Make cache keys of other Params a Memoize subplan depends on :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -589,15 +589,42 @@ paraminfo_get_equal_hashops(PlannerInfo *root, ParamPathInfo *param_info,
 	}
 
 	/*
-	 * A Param buried inside a larger cache key expression isn't determined by
-	 * the key's value, so create_memoize_plan() arranges for the cache to be
-	 * purged whenever it changes, which commonly happens on every rescan of
-	 * the enclosing plan.  We're better off making a cache key of the Param
-	 * by itself so that the cached results survive.  This is only an
-	 * optimization, so we needn't refuse to memoize when one isn't hashable.
+	 * A Param which the inner scan uses other than as a cache key by itself,
+	 * whether buried inside a larger cache key expression or used elsewhere,
+	 * isn't determined by the cache keys, so create_memoize_plan() arranges
+	 * for the cache to be purged whenever it changes, which commonly happens
+	 * on every rescan of the enclosing plan.  We're better off making a cache
+	 * key of the Param by itself so that the cached results survive.  This is
+	 * only an optimization, so we needn't track down every last such Param,
+	 * nor refuse to memoize when one isn't hashable.
 	 */
 	params = NIL;
 	(void) pull_exec_params_walker((Node *) *param_exprs, &params);
+	foreach(lc, innerrel->baserestrictinfo)
+	{
+		RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);
+
+		(void) pull_exec_params_walker((Node *) rinfo->clause, &params);
+	}
+	(void) pull_exec_params_walker((Node *) innerrel->reltarget->exprs, &params);
+
+	/*
+	 * Only the outer side of each ppi_clause became a cache key above, so the
+	 * inner sides must be examined here.
+	 */
+	if (param_info != NULL)
+	{
+		foreach(lc, param_info->ppi_clauses)
+		{
+			RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);
+			OpExpr	   *opexpr = (OpExpr *) rinfo->clause;
+
+			(void) pull_exec_params_walker(rinfo->outer_is_left ?
+										   (Node *) lsecond(opexpr->args) :
+										   (Node *) linitial(opexpr->args),
+										   &params);
+		}
+	}
 
 	foreach(lc, params)
 		(void) memoize_add_cache_key((Node *) lfirst(lc), param_exprs,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_difference`
- 外部函数：`bms_join`
- 外部函数：`bms_overlap`
- 外部函数：`create_memoize_plan`
- 外部函数：`exprType`
- 外部函数：`expression_tree_walker`
- 外部函数：`lappend`
- 外部函数：`lappend_oid`
- 外部函数：`lfirst`
- 外部函数：`list_member`
- 外部函数：`lookup_type_cache`
- 外部函数：`memoize_add_cache_key`
- 大写宏：`JOIN_FULL`
- 大写宏：`LHS`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`PARAM_EXEC`
- 大写宏：`RHS`
- 大写宏：`TYPECACHE_EQ_OPR`
- 大写宏：`TYPECACHE_HASH_PROC`
- 外部类型：`If`
- 外部类型：`List`
- 外部类型：`Node`
- 外部类型：`Param`
- 外部类型：`Params`
- 外部类型：`See`
- 外部类型：`SpecialJoinInfo`
- 外部类型：`This`
- 外部类型：`TypeCacheEntry`
- 外部类型：`Vars`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f7112ca834` → 本草稿移入 `cases/defect/auto-postgres-f7112ca834/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
