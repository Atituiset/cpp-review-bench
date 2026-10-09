# auto-postgres-7b5fed73e1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#0f7dfd7936b9e99f9aeca00f52ab1692898ffff4](https://github.com/postgres/postgres/commit/0f7dfd7936b9e99f9aeca00f52ab1692898ffff4) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 124 |
| 编译错误数（gcc syntax-only） | 63（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0f7dfd7936b9e99f9aeca00f52ab1692898ffff4 (https://github.com/postgres/postgres/commit/0f7dfd7936b9e99f9aeca00f52ab1692898ffff4)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0f7dfd7936b9e99f9aeca00f52ab1692898ffff4 Fix failure of setrefs.c to process child_append_relid_sets :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -155,6 +155,7 @@ static Plan *set_mergeappend_references(PlannerInfo *root,
 										int rtoffset);
 static void set_hash_references(PlannerInfo *root, Plan *plan, int rtoffset);
 static Relids offset_relid_set(Relids relids, int rtoffset);
+static List *offset_relid_set_list(List *relid_sets, int rtoffset);
 static Node *fix_dummy_setop_vars_mutator(Node *node, int *first_child_relid);
 static Node *fix_scan_expr(PlannerInfo *root, Node *node,
 						   int rtoffset, double num_exec);
@@ -1913,6 +1914,8 @@ set_append_references(PlannerInfo *root,
 	set_dummy_tlist_references((Plan *) aplan, rtoffset);
 
 	aplan->apprelids = offset_relid_set(aplan->apprelids, rtoffset);
+	aplan->child_append_relid_sets =
+		offset_relid_set_list(aplan->child_append_relid_sets, rtoffset);
 
 	/*
 	 * Add PartitionPruneInfo, if any, to PlannerGlobal and update the index.
@@ -1991,6 +1994,8 @@ set_mergeappend_references(PlannerInfo *root,
 	set_dummy_tlist_references((Plan *) mplan, rtoffset);
 
 	mplan->apprelids = offset_relid_set(mplan->apprelids, rtoffset);
+	mplan->child_append_relid_sets =
+		offset_relid_set_list(mplan->child_append_relid_sets, rtoffset);
 
 	/*
 	 * Add PartitionPruneInfo, if any, to PlannerGlobal and update the index.
@@ -2052,6 +2057,24 @@ offset_relid_set(Relids relids, int rtoffset)
 	return bms_offset_members(relids, rtoffset);
 }
 
+/*
+ * offset_relid_set_list
+ *		Apply rtoffset to the members of each Relid set in a List.
+ */
+static List *
+offset_relid_set_list(List *relid_sets, int rtoffset)
+{
+	List	   *result = NIL;
+
+	if (rtoffset == 0)
+		return relid_sets;
+
+	foreach_ptr(Bitmapset, relids, relid_sets)
+		result = lappend(result, offset_relid_set(relids, rtoffset));
+
+	return result;
+}
+
 /*
  * copyVar
  *		Copy a Var node.
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_offset_members`
- 外部函数：`build_tlist_index`
- 外部函数：`elog`
- 外部函数：`equal`
- 外部函数：`fix_dummy_setop_vars_mutator`
- 外部函数：`fix_scan_expr`
- 外部函数：`fix_upper_expr`
- 外部函数：`lappend_int`
- 外部函数：`lappend_oid`
- 外部函数：`lfirst_int`
- 外部函数：`list_length`
- 外部函数：`list_nth`
- 外部函数：`palloc_object`
- 外部函数：`plan`
- 外部函数：`record_plan_function_dependency`
- 外部函数：`set_dummy_tlist_references`
- 外部函数：`set_opfuncid`
- 外部函数：`set_sa_opfuncid`
- 大写宏：`ERROR`
- 大写宏：`FLEXIBLE_ARRAY_MEMBER`
- 大写宏：`NIL`
- 大写宏：`NRM_EQUAL`
- 大写宏：`NRM_SUPERSET`
- 大写宏：`NULL`
- 大写宏：`OIDOID`
- 大写宏：`OUTER_VAR`
- 大写宏：`PARAM_MULTIE`
- 大写宏：`PARAM_MULTIEXPR`
- 大写宏：`REGCLASSOID`
- 大写宏：`TLE`
- 外部类型：`Add`
- 外部类型：`Aggref`
- 外部类型：`AttrNumber`
- 外部类型：`Bitmapset`
- 外部类型：`Check`
- 外部类型：`Const`
- 外部类型：`Copy`
- 外部类型：`DistinctExpr`
- 外部类型：`FuncExpr`
- 外部类型：`GroupingFunc`
- 外部类型：`Hash`
- 外部类型：`HashJoin`
- 外部类型：`If`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Node`
- 外部类型：`NullIfExpr`
- 外部类型：`OpExpr`
- 外部类型：`Param`
- 外部类型：`PartitionPruneInfo`
- 外部类型：`PlaceHolderVar`
- 外部类型：`Plan`
- 外部类型：`PlannerGlobal`
- 外部类型：`PlannerInfo`
- 外部类型：`Relids`
- 外部类型：`ScalarArrayOpExpr`
- 外部类型：`Var`
- 外部类型：`We`
- 外部类型：`WindowFunc`
- 外部类型：`equivalence`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-7b5fed73e1` → 本草稿移入 `cases/defect/auto-postgres-7b5fed73e1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
