# auto-postgres-4a7899776d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#264ddc9a678c41fca3435f26d7c581fd2b034cf8](https://github.com/postgres/postgres/commit/264ddc9a678c41fca3435f26d7c581fd2b034cf8) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 38 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #264ddc9a678c41fca3435f26d7c581fd2b034cf8 (https://github.com/postgres/postgres/commit/264ddc9a678c41fca3435f26d7c581fd2b034cf8)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 6547；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 264ddc9a678c41fca3435f26d7c581fd2b034cf8 Fix qual pushdown past grouping through simple CASE :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -103,12 +103,15 @@ typedef struct
 /*
  * Walker context for expression_has_grouping_conflict.  get_eqop is a callback
  * that returns the equality operator used for grouping.  cb_context is opaque
- * to the walker and is forwarded to get_eqop unchanged.
+ * to the walker and is forwarded to get_eqop unchanged.  case_var is the Var
+ * that the CaseTestExprs of the simple CASE being walked stand for, or NULL if
+ * there is none.
  */
 typedef struct
 {
 	grouping_eqop_callback get_eqop;
 	void	   *cb_context;
+	Var		   *case_var;
 } grouping_walker_ctx;
 
 static bool contain_agg_clause_walker(Node *node, void *context);
@@ -6438,6 +6441,7 @@ expression_has_grouping_conflict(Node *expr,
 
 	ctx.get_eqop = get_eqop;
 	ctx.cb_context = context;
+	ctx.case_var = NULL;
 
 	return grouping_conflict_walker(expr, &ctx);
 }
@@ -6456,10 +6460,13 @@ expression_has_grouping_conflict(Node *expr,
  * member, and RowCompareExpr (one operator and collation per column).  A
  * simple CASE (CaseExpr with a non-NULL arg) is a comparison in disguise:
  * parse analysis builds each WHEN as "OpExpr(CaseTestExpr op val)", with the
- * CaseTestExpr standing in for the arg, so the arg is effectively an operand
- * of each WHEN's comparison.  Those WHEN operators are always the type-default
- * "=", matching the grouping eqop, so only a collation conflict is possible
- * there.
+ * CaseTestExpr standing in for the arg.  If the arg is a Var (after looking
+ * through RelabelType), it is bound in ctx->case_var while the WHEN
+ * conditions are walked and each CaseTestExpr is resolved to it, so the Var
+ * is checked exactly as each WHEN uses it.  Any other arg is walked once as
+ * a non-operand and its CaseTestExprs are ignored, as are those in an
+ * ArrayCoerceExpr's elemexpr and a JsonConstructorExpr's coercion, which
+ * stand for something else.
  */
 static bool
 grouping_conflict_walker(Node *node, grouping_walker_ctx *ctx)
@@ -6526,54 +6533,88 @@ grouping_conflict_walker(Node *node, grouping_walker_ctx *ctx)
 		}
 		return false;
 	}
+	else if (IsA(node, CaseTestExpr))
+	{
+		/*
+		 * A direct operand of a comparison is handled by
+		 * grouping_check_operand; any other use is a non-operand reference to
+		 * the Var it stands for, if any.
+		 */
+		return grouping_conflict_walker((Node *) ctx->case_var, ctx);
+	}
+	else if (IsA(node, ArrayCoerceExpr))
+	{
+		ArrayCoerceExpr *acexpr = (ArrayCoerceExpr *) node;
+		Var		   *save_case_var = ctx->case_var;
+		bool		result;
+
+		if (grouping_conflict_walker((Node *) acexpr->arg, ctx))
+			return true;
+
+		/* The CaseTestExpr in elemexpr is an array element, not case_var. */
+		ctx->case_var = NULL;
+		result = grouping_conflict_walker((Node *) acexpr->elemexpr, ctx);
+		ctx->case_var = save_case_var;
+		return result;
+	}
+	else if (IsA(node, JsonConstructorExpr))
+	{
+		JsonConstructorExpr *ctor = (JsonConstructorExpr *) node;
+		Var		   *save_case_var = ctx->case_var;
+		bool		result;
+
+		if (grouping_conflict_walker((Node *) ctor->args, ctx))
+			return true;
+		if (grouping_conflict_walker((Node *) ctor->func, ctx))
+			return true;
+
+		/* The CaseTestExpr in coercion is the JSON result, not case_var. */
+		ctx->case_var = NULL;
+		result = grouping_conflict_walker((Node *) ctor->coercion, ctx);
+		ctx->case_var = save_case_var;
+		return result;
+	}
 	else if (IsA(node, CaseExpr) && ((CaseExpr *) node)->arg != NULL)
 	{
 		CaseExpr   *cexpr = (CaseExpr *) node;
 		Node	   *arg = (Node *) cexpr->arg;
+		Var		   *save_case_var = ctx->case_var;
+		bool		result = false;
 
 		/* Look through RelabelType to find a direct Var arg. */
 		while (arg && IsA(arg, RelabelType))
 			arg = (Node *) ((RelabelType *) arg)->arg;
 
+		/*
+		 * A Var arg needs no walk of its own: each WHEN condition refers to
+		 * it through a CaseTestExpr, which is resolved to the Var and checked
+		 * as the WHEN uses it.  Any other arg is a non-operand reference in
+		 * its own right: walk it once here and ignore it
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`above`
- 外部函数：`column`
- 外部函数：`exprInputCollation`
- 外部函数：`expression_tree_walker`
- 外部函数：`get_collation_isdeterministic`
- 外部函数：`get_eqop`
- 外部函数：`grouping_check_operand`
- 外部函数：`lfirst`
- 外部函数：`lfirst_oid`
- 外部函数：`op_is_safe_index_member`
- 大写宏：`CASE`
- 大写宏：`NULL`
- 大写宏：`WHEN`
- 外部类型：`Aggref`
- 外部类型：`Any`
- 外部类型：`CaseExpr`
- 外部类型：`CaseTestExpr`
- 外部类型：`CaseWhen`
- 外部类型：`Each`
- 外部类型：`For`
- 外部类型：`GroupingFunc`
- 外部类型：`Handle`
- 外部类型：`If`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Look`
- 外部类型：`Node`
- 外部类型：`Oid`
- 外部类型：`OpExpr`
- 外部类型：`RelabelType`
- 外部类型：`RowCompareExpr`
- 外部类型：`ScalarArrayOpExpr`
- 外部类型：`SubLink`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`Those`
- 外部类型：`Var`
- 外部类型：`Walk`
- 外部类型：`Walker`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-4a7899776d` → 本草稿移入 `cases/defect/auto-postgres-4a7899776d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
