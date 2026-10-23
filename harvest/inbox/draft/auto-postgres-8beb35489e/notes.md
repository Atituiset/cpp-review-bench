# auto-postgres-8beb35489e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1910daf98317bc321562e7d7436a791538825d0e](https://github.com/postgres/postgres/commit/1910daf98317bc321562e7d7436a791538825d0e) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 84 |
| 编译错误数（gcc syntax-only） | 34（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1910daf98317bc321562e7d7436a791538825d0e (https://github.com/postgres/postgres/commit/1910daf98317bc321562e7d7436a791538825d0e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 102；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1910daf98317bc321562e7d7436a791538825d0e Fix handling of some SQL/JSON locations in query jumbling :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -99,7 +99,7 @@ static JsonBehavior *transformJsonBehavior(ParseState *pstate, JsonExpr *jsexpr,
 										   JsonBehavior *behavior,
 										   JsonBehaviorType default_behavior,
 										   JsonReturning *returning);
-static Node *GetJsonBehaviorConst(JsonBehaviorType btype, int location);
+static Node *GetJsonBehaviorConst(JsonBehaviorType btype);
 static Node *make_row_comparison_op(ParseState *pstate, List *opname,
 									List *largs, List *rargs, int location);
 static Node *make_row_distinct_op(ParseState *pstate, List *opname,
@@ -4897,7 +4897,7 @@ transformJsonBehavior(ParseState *pstate, JsonExpr *jsexpr,
 	}
 
 	if (expr == NULL && btype != JSON_BEHAVIOR_ERROR)
-		expr = GetJsonBehaviorConst(btype, location);
+		expr = GetJsonBehaviorConst(btype);
 
 	/*
 	 * Try to coerce the expression if needed.
@@ -4988,7 +4988,7 @@ transformJsonBehavior(ParseState *pstate, JsonExpr *jsexpr,
 							errmsg("cannot cast behavior expression of type %s to %s",
 								   format_type_be(exprType(expr)),
 								   format_type_be(returning->typid)),
-							parser_errposition(pstate, exprLocation(expr)));
+							parser_errposition(pstate, location));
 			}
 
 			expr = coerced_expr;
@@ -5010,7 +5010,7 @@ transformJsonBehavior(ParseState *pstate, JsonExpr *jsexpr,
  * JsonBehaviorType.
  */
 static Node *
-GetJsonBehaviorConst(JsonBehaviorType btype, int location)
+GetJsonBehaviorConst(JsonBehaviorType btype)
 {
 	Datum		val = (Datum) 0;
 	Oid			typid = JSONBOID;
@@ -5065,7 +5065,6 @@ GetJsonBehaviorConst(JsonBehaviorType btype, int location)
 	}
 
 	con = makeConst(typid, -1, InvalidOid, len, val, isnull, isbyval);
-	con->location = location;
 
 	return (Node *) con;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_member`
- 外部函数：`bms_int_members`
- 外部函数：`bms_next_member`
- 外部函数：`castNode`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errdetail`
- 外部函数：`errhint`
- 外部函数：`errmsg`
- 外部函数：`expression_returns_set`
- 外部函数：`format_type_be`
- 外部函数：`get_op_index_interpretation`
- 外部函数：`interpretations`
- 外部函数：`lappend`
- 外部函数：`lappend_oid`
- 外部函数：`lfirst`
- 外部函数：`linitial`
- 外部函数：`list_length`
- 外部函数：`llast`
- 外部函数：`lsecond`
- 外部函数：`makeBoolExpr`
- 外部函数：`makeNode`
- 外部函数：`make_op`
- 外部函数：`make_row_comparison_op`
- 外部函数：`operator`
- 外部函数：`palloc_array`
- 外部函数：`parser_errposition`
- 外部函数：`semantics`
- 外部函数：`strVal`
- 大写宏：`AND`
- 大写宏：`AND_EXPR`
- 大写宏：`BOOLOID`
- 大写宏：`COMPARE_EQ`
- 大写宏：`COMPARE_NE`
- 大写宏：`ERRCODE_DATATYPE_MISMATCH`
- 大写宏：`ERRCODE_FEATURE_NOT_SUPPORTED`
- 大写宏：`ERRCODE_SYNTAX_ERROR`
- 大写宏：`ERROR`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`OR_EXPR`
- 外部类型：`Bitmapset`
- 外部类型：`CompareType`
- 外部类型：`For`
- 外部类型：`Identify`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`InvalidOid`
- 外部类型：`JsonBehavior`
- 外部类型：`JsonBehaviorType`
- 外部类型：`JsonReturning`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`No`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Oid`
- 外部类型：`OpExpr`
- 外部类型：`OpExprs`
- 外部类型：`OpIndexInterpretation`
- 外部类型：`Otherwise`
- 外部类型：`ParseState`
- 外部类型：`Row`
- 外部类型：`RowCompareExpr`
- 外部类型：`There`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-8beb35489e` → 本草稿移入 `cases/defect/auto-postgres-8beb35489e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
