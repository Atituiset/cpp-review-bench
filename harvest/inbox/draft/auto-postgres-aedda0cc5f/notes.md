# auto-postgres-aedda0cc5f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#792094a5ce89f9953d913c835e4530e56b63026a](https://github.com/postgres/postgres/commit/792094a5ce89f9953d913c835e4530e56b63026a) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 56 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #792094a5ce89f9953d913c835e4530e56b63026a (https://github.com/postgres/postgres/commit/792094a5ce89f9953d913c835e4530e56b63026a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 254；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 792094a5ce89f9953d913c835e4530e56b63026a Coerce GRAPH_TABLE pattern WHERE clauses to boolean :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -22,6 +22,7 @@
 #include "catalog/pg_propgraph_property.h"
 #include "miscadmin.h"
 #include "nodes/makefuncs.h"
+#include "parser/parse_clause.h"
 #include "parser/parse_collate.h"
 #include "parser/parse_expr.h"
 #include "parser/parse_graphtable.h"
@@ -251,7 +252,8 @@ transformGraphElementPattern(ParseState *pstate, GraphElementPattern *gep)
 
 	gep->labelexpr = transformLabelExpr(gpstate, gep->labelexpr);
 
-	gep->whereClause = transformExpr(pstate, gep->whereClause, EXPR_KIND_WHERE);
+	gep->whereClause = transformWhereClause(pstate, gep->whereClause,
+											EXPR_KIND_WHERE, "WHERE");
 
 	/*
 	 * Assign collations here for the reason mentioned in the prologue of
@@ -387,7 +389,8 @@ transformGraphPattern(ParseState *pstate, GraphPattern *graph_pattern)
 											 transformPathPatternList(pstate, graph_pattern->path_pattern_list));
 
 	graph_pattern->path_pattern_list = path_pattern_list;
-	graph_pattern->whereClause = transformExpr(pstate, graph_pattern->whereClause, EXPR_KIND_WHERE);
+	graph_pattern->whereClause = transformWhereClause(pstate, graph_pattern->whereClause,
+													  EXPR_KIND_WHERE, "WHERE");
 	assign_expr_collations(pstate, graph_pattern->whereClause);
 
 	return (Node *) graph_pattern;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assign_expr_collations`
- 外部函数：`check_stack_depth`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`get_rel_name`
- 外部函数：`lappend`
- 外部函数：`lfirst`
- 外部函数：`linitial`
- 外部函数：`list_append_unique`
- 外部函数：`list_length`
- 外部函数：`makeBoolExpr`
- 外部函数：`makeNode`
- 外部函数：`makeString`
- 外部函数：`nodeTag`
- 外部函数：`parser_errposition`
- 外部函数：`pstrdup`
- 外部函数：`strVal`
- 外部函数：`transformExpr`
- 外部函数：`transformGraphPattern`
- 大写宏：`EDGE_PATTERN_ANY`
- 大写宏：`EDGE_PATTERN_LEFT`
- 大写宏：`EDGE_PATTERN_RIGHT`
- 大写宏：`ERRCODE_FEATURE_NOT_SUPPORTED`
- 大写宏：`ERRCODE_UNDEFINED_OBJECT`
- 大写宏：`ERROR`
- 大写宏：`EXPR_KIND_WHERE`
- 大写宏：`GRAPH_TABLE`
- 大写宏：`IS_EDGE_PATTERN`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`PAREN_EXPR`
- 大写宏：`PROPGRAPHLABELNAME`
- 大写宏：`VERTEX_PATTERN`
- 大写宏：`WHERE`
- 外部类型：`Anum_pg_propgraph_label_oid`
- 外部类型：`Assign`
- 外部类型：`BoolExpr`
- 外部类型：`But`
- 外部类型：`Collect`
- 外部类型：`ColumnRef`
- 外部类型：`Grammar`
- 外部类型：`GraphElementPattern`
- 外部类型：`GraphElementPatternKind`
- 外部类型：`GraphLabelRef`
- 外部类型：`GraphTableParseState`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Node`
- 外部类型：`Oid`
- 外部类型：`ParseState`
- 外部类型：`Path`
- 外部类型：`T_BoolExpr`
- 外部类型：`T_ColumnRef`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-aedda0cc5f` → 本草稿移入 `cases/defect/auto-postgres-aedda0cc5f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
