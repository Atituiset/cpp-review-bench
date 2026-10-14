# auto-postgres-5907f87927

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b28a2b2bfa50cb7aa3fcea069f0afad0ebcb6491](https://github.com/postgres/postgres/commit/b28a2b2bfa50cb7aa3fcea069f0afad0ebcb6491) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 50 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b28a2b2bfa50cb7aa3fcea069f0afad0ebcb6491 (https://github.com/postgres/postgres/commit/b28a2b2bfa50cb7aa3fcea069f0afad0ebcb6491)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 607；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b28a2b2bfa50cb7aa3fcea069f0afad0ebcb6491 Fix two errors in extraction of GIN indexquals from jsonpath queries. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -558,6 +558,7 @@ extract_jsp_path_expr_nodes(JsonPathGinContext *cxt, JsonPathGinPath path,
  * Extract an expression node from one of following jsonpath path expressions:
  *   EXISTS(jsp)    (when 'scalar' is NULL)
  *   jsp == scalar  (when 'scalar' is not NULL).
+ * Returns NULL if no usable filter expression can be extracted.
  *
  * The current path (@) is passed in 'path'.
  */
@@ -579,7 +580,10 @@ extract_jsp_path_expr(JsonPathGinContext *cxt, JsonPathGinPath path,
 	return make_jsp_expr_node_args(JSP_GIN_AND, nodes);
 }
 
-/* Recursively extract nodes from the boolean jsonpath expression. */
+/*
+ * Recursively extract a GIN qual from the boolean jsonpath expression.
+ * Returns NULL if no usable indexqual can be extracted.
+ */
 static JsonPathGinNode *
 extract_jsp_bool_expr(JsonPathGinContext *cxt, JsonPathGinPath path,
 					  JsonPathItem *jsp, bool not)
@@ -591,6 +595,8 @@ extract_jsp_bool_expr(JsonPathGinContext *cxt, JsonPathGinPath path,
 		case jpiAnd:			/* expr && expr */
 		case jpiOr:				/* expr || expr */
 			{
+				/* We use De Morgan's laws to handle a passed-down NOT */
+				bool		is_and = (not ^ (jsp->type == jpiAnd));
 				JsonPathItem arg;
 				JsonPathGinNode *larg;
 				JsonPathGinNode *rarg;
@@ -604,13 +610,19 @@ extract_jsp_bool_expr(JsonPathGinContext *cxt, JsonPathGinPath path,
 
 				if (!larg || !rarg)
 				{
-					if (jsp->type == jpiOr)
+					/*
+					 * For OR, we have to fail if either arm is unsupported.
+					 * For AND, we can just ignore the unsupported arm and
+					 * apply the supported one, relying on indexscan qual
+					 * recheck to reject rows that don't match the other arm.
+					 */
+					if (!is_and)
 						return NULL;
 
 					return larg ? larg : rarg;
 				}
 
-				type = not ^ (jsp->type == jpiAnd) ? JSP_GIN_AND : JSP_GIN_OR;
+				type = is_and ? JSP_GIN_AND : JSP_GIN_OR;
 
 				return make_jsp_expr_node_binary(type, larg, rarg);
 			}
@@ -662,24 +674,28 @@ extract_jsp_bool_expr(JsonPathGinContext *cxt, JsonPathGinPath path,
 				JsonPathItem *scalar_item;
 				JsonbValue	scalar;
 
+				/* Can't support !(x == y) */
 				if (not)
 					return NULL;
 
 				jspGetLeftArg(jsp, &left_item);
 				jspGetRightArg(jsp, &right_item);
 
-				if (jspIsScalar(left_item.type))
+				/* To build an indexqual, one side must be a simple scalar */
+				if (jspIsScalar(left_item.type) &&
+					!jspHasNext(&left_item))
 				{
 					scalar_item = &left_item;
 					path_item = &right_item;
 				}
-				else if (jspIsScalar(right_item.type))
+				else if (jspIsScalar(right_item.type) &&
+						 !jspHasNext(&right_item))
 				{
 					scalar_item = &right_item;
 					path_item = &left_item;
 				}
 				else
-					return NULL;	/* at least one operand should be a scalar */
+					return NULL;
 
 				switch (scalar_item->type)
 				{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`add_path_item`
- 外部函数：`check_stack_depth`
- 外部函数：`extract_jsp_path_expr`
- 外部函数：`extract_nodes`
- 外部函数：`foreach`
- 外部函数：`item`
- 外部函数：`items`
- 外部函数：`jspGetArg`
- 外部函数：`jspGetLeftArg`
- 外部函数：`jspGetNext`
- 外部函数：`jspGetRightArg`
- 外部函数：`jspIsScalar`
- 外部函数：`keyvalue`
- 外部函数：`lappend`
- 外部函数：`lfirst`
- 外部函数：`linitial`
- 外部函数：`list_length`
- 外部函数：`name`
- 外部函数：`offsetof`
- 外部函数：`palloc`
- 外部函数：`path`
- 大写宏：`AND`
- 大写宏：`EMPTY`
- 大写宏：`EXISTS`
- 大写宏：`FOR_ALL`
- 大写宏：`GIN`
- 大写宏：`JSP_GIN_AND`
- 大写宏：`JSP_GIN_ENTRY`
- 大写宏：`JSP_GIN_OR`
- 大写宏：`NIL`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 外部类型：`Append`
- 外部类型：`Datum`
- 外部类型：`Extract`
- 外部类型：`However`
- 外部类型：`JsonPathGinContext`
- 外部类型：`JsonPathGinNode`
- 外部类型：`JsonPathGinPath`
- 外部类型：`JsonPathGinPathItem`
- 外部类型：`JsonPathItem`
- 外部类型：`JsonPathItemType`
- 外部类型：`JsonbValue`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Path`
- 外部类型：`Recursively`
- 外部类型：`So`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-5907f87927` → 本草稿移入 `cases/defect/auto-postgres-5907f87927/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
