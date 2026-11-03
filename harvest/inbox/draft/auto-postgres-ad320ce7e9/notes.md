# auto-postgres-ad320ce7e9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#45ff0b4a317f14539567a3500eb41ec8b06cde3d](https://github.com/postgres/postgres/commit/45ff0b4a317f14539567a3500eb41ec8b06cde3d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 29 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #45ff0b4a317f14539567a3500eb41ec8b06cde3d (https://github.com/postgres/postgres/commit/45ff0b4a317f14539567a3500eb41ec8b06cde3d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 630；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 45ff0b4a317f14539567a3500eb41ec8b06cde3d Make Memoize cache keys only of Params from outer query levels :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -487,6 +487,8 @@ paraminfo_get_equal_hashops(PlannerInfo *root, ParamPathInfo *param_info,
 {
 	List	   *lateral_vars;
 	List	   *params;
+	Bitmapset  *outer_paramids;
+	PlannerInfo *proot;
 	ListCell   *lc;
 
 	*param_exprs = NIL;
@@ -597,7 +599,30 @@ paraminfo_get_equal_hashops(PlannerInfo *root, ParamPathInfo *param_info,
 	 * key of the Param by itself so that the cached results survive.  This is
 	 * only an optimization, so we needn't track down every last such Param,
 	 * nor refuse to memoize when one isn't hashable.
+	 *
+	 * Only Params supplied by an outer query level are safe to evaluate as
+	 * cache keys.  Others, such as the output Params of initPlans and
+	 * SubPlans, may not have a value yet.  Outer query levels list the Params
+	 * they supply in their plan_params.
 	 */
+	if (root->glob->paramExecTypes == NIL)
+		return true;			/* no PARAM_EXEC Params at all */
+
+	outer_paramids = NULL;
+	for (proot = root->parent_root; proot != NULL; proot = proot->parent_root)
+	{
+		foreach(lc, proot->plan_params)
+		{
+			PlannerParamItem *pitem = lfirst_node(PlannerParamItem, lc);
+
+			outer_paramids = bms_add_member(outer_paramids, pitem->paramId);
+		}
+	}
+
+	/* Nothing to do if no outer level supplies any Params */
+	if (outer_paramids == NULL)
+		return true;
+
 	params = NIL;
 	(void) pull_exec_params_walker((Node *) *param_exprs, &params);
 	foreach(lc, innerrel->baserestrictinfo)
@@ -627,8 +652,13 @@ paraminfo_get_equal_hashops(PlannerInfo *root, ParamPathInfo *param_info,
 	}
 
 	foreach(lc, params)
-		(void) memoize_add_cache_key((Node *) lfirst(lc), param_exprs,
-									 operators, binary_mode);
+	{
+		Param	   *param = lfirst_node(Param, lc);
+
+		if (bms_is_member(param->paramid, outer_paramids))
+			(void) memoize_add_cache_key((Node *) param, param_exprs,
+										 operators, binary_mode);
+	}
 
 	/* We're okay to use memoize */
 	return true;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_difference`
- 外部函数：`bms_join`
- 外部函数：`bms_overlap`
- 外部函数：`exprType`
- 外部函数：`expression_tree_walker`
- 外部函数：`lappend`
- 外部函数：`lappend_oid`
- 外部函数：`lfirst`
- 外部函数：`list_member`
- 外部函数：`lookup_type_cache`
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
- 外部类型：`ListCell`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ad320ce7e9` → 本草稿移入 `cases/defect/auto-postgres-ad320ce7e9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
