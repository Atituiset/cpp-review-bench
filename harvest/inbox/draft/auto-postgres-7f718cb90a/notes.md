# auto-postgres-7f718cb90a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b6dae498dfa60c9d15abf791d51434e8d23e5cae](https://github.com/postgres/postgres/commit/b6dae498dfa60c9d15abf791d51434e8d23e5cae) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 109 |
| 编译错误数（gcc syntax-only） | 54（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b6dae498dfa60c9d15abf791d51434e8d23e5cae (https://github.com/postgres/postgres/commit/b6dae498dfa60c9d15abf791d51434e8d23e5cae)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 4276；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b6dae498dfa60c9d15abf791d51434e8d23e5cae Qualify unqualified names in CREATE SCHEMA elements again. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -136,7 +136,7 @@ static void transformCheckConstraints(CreateStmtContext *cxt,
 static void transformConstraintAttrs(ParseState *pstate,
 									 List *constraintList);
 static void transformColumnType(CreateStmtContext *cxt, ColumnDef *column);
-static void checkSchemaNameRV(CreateSchemaStmtContext *cxt, RangeVar *relation);
+static void setSchemaNameRV(CreateSchemaStmtContext *cxt, RangeVar *relation);
 static CreateStmt *transformCreateSchemaCreateTable(ParseState *pstate,
 													CreateStmt *stmt,
 													List **fk_elements);
@@ -4127,7 +4127,7 @@ transformColumnType(CreateStmtContext *cxt, ColumnDef *column)
  * transformCreateSchemaStmtElements -
  *	  analyzes the elements of a CREATE SCHEMA statement
  *
- * This presently has two responsibilities.  We verify that no subcommands
+ * This presently has two responsibilities.  We ensure that no subcommands
  * are trying to create objects outside the new schema.  We also attempt to
  * re-order the subcommands such that there are no forward references
  * (e.g. GRANT to a table created later in the list).  Note that the logic
@@ -4159,6 +4159,9 @@ transformCreateSchemaStmtElements(ParseState *pstate, List *schemaElts,
 	List	   *result;
 	ListCell   *elements;
 
+	/* We fill in schema names below, so work on a copy */
+	schemaElts = copyObject(schemaElts);
+
 	cxt.pstate = pstate;
 	cxt.schemaname = schemaName;
 	cxt.sequences = NIL;
@@ -4170,9 +4173,8 @@ transformCreateSchemaStmtElements(ParseState *pstate, List *schemaElts,
 	cxt.foreign_keys = NIL;
 
 	/*
-	 * Run through each schema element in the schema element list.  Check
-	 * target schema names, separate statements by type, and do preliminary
-	 * analysis.
+	 * Run through each schema element in the schema element list.  Set target
+	 * schema names, separate statements by type, and do preliminary analysis.
 	 */
 	foreach(elements, schemaElts)
 	{
@@ -4184,7 +4186,7 @@ transformCreateSchemaStmtElements(ParseState *pstate, List *schemaElts,
 				{
 					CreateSeqStmt *elp = (CreateSeqStmt *) element;
 
-					checkSchemaNameRV(&cxt, elp->sequence);
+					setSchemaNameRV(&cxt, elp->sequence);
 					cxt.sequences = lappend(cxt.sequences, element);
 				}
 				break;
@@ -4193,7 +4195,7 @@ transformCreateSchemaStmtElements(ParseState *pstate, List *schemaElts,
 				{
 					CreateStmt *elp = (CreateStmt *) element;
 
-					checkSchemaNameRV(&cxt, elp->relation);
+					setSchemaNameRV(&cxt, elp->relation);
 					/* Pull out any foreign key clauses, add to fk_elements */
 					elp = transformCreateSchemaCreateTable(pstate,
 														   elp,
@@ -4210,7 +4212,7 @@ transformCreateSchemaStmtElements(ParseState *pstate, List *schemaElts,
 				{
 					ViewStmt   *elp = (ViewStmt *) element;
 
-					checkSchemaNameRV(&cxt, elp->view);
+					setSchemaNameRV(&cxt, elp->view);
 
 					/*
 					 * XXX todo: deal with references between views
@@ -4223,7 +4225,7 @@ transformCreateSchemaStmtElements(ParseState *pstate, List *schemaElts,
 				{
 					IndexStmt  *elp = (IndexStmt *) element;
 
-					checkSchemaNameRV(&cxt, elp->relation);
+					setSchemaNameRV(&cxt, elp->relation);
 					cxt.indexes = lappend(cxt.indexes, element);
 				}
 				break;
@@ -4232,7 +4234,7 @@ transformCreateSchemaStmtElements(ParseState *pstate, List *schemaElts,
 				{
 					CreateTrigStmt *elp = (CreateTrigStmt *) element;
 
-					checkSchemaNameRV(&cxt, elp->relation);
+					setSchemaNameRV(&cxt, elp->relation);
 					cxt.triggers = lappend(cxt.triggers, element);
 				}
 				break;
@@ -4260,21 +4262,21 @@ transformCreateSchemaStmtElements(ParseState *pstate, List *schemaElts,
 }
 
 /*
- * checkSchemaNameRV
- *		Check schema name in an element of a CREATE SCHEMA command,
+ * setSchemaNameRV
+ *		Set or check schema name in an element of a CREATE SCHEMA command,
  *		where the element's name is given by a RangeVar
  *
- * It's okay if the command doesn't specify a target schema name, because
- * CreateSchemaCommand will set up the d
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`attributes`
- 外部函数：`copyObject`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`foreach_delete_current`
- 外部函数：`format_type_be`
- 外部函数：`lappend`
- 外部函数：`lfirst`
- 外部函数：`list_make1`
- 外部函数：`makeNode`
- 外部函数：`makeString`
- 外部函数：`memcpy`
- 外部函数：`nodeTag`
- 外部函数：`parser_errposition`
- 外部函数：`schema`
- 外部函数：`strcmp`
- 外部函数：`transformCreateSchemaCreateTable`
- 外部函数：`typenameType`
- 大写宏：`ADD`
- 大写宏：`ALTER`
- 大写宏：`CHECK`
- 大写宏：`COLLATE`
- 大写宏：`CONSTRAINT`
- 大写宏：`CONSTR_ATTR_DEFERRABLE`
- 大写宏：`CONSTR_ATTR_DEFERRED`
- 大写宏：`CONSTR_ATTR_ENFORCED`
- 大写宏：`CONSTR_ATTR_IMMEDIATE`
- 大写宏：`CONSTR_ATTR_NOT_DEFERRABLE`
- 大写宏：`CONSTR_ATTR_NOT_ENFORCED`
- 大写宏：`CONSTR_CHECK`
- 大写宏：`CONSTR_EXCLUSION`
- 大写宏：`CONSTR_FOREIGN`
- 大写宏：`CONSTR_PRIMARY`
- 大写宏：`CONSTR_UNIQUE`
- 大写宏：`CREATE`
- 大写宏：`DEFERRABLE`
- 大写宏：`DEFERRED`
- 大写宏：`ENFORCED`
- 大写宏：`ERRCODE_DATATYPE_MISMATCH`
- 大写宏：`ERRCODE_INVALID_SCHEMA_DEFINITION`
- 大写宏：`ERRCODE_SYNTAX_ERROR`
- 大写宏：`ERROR`
- 大写宏：`FOR`
- 大写宏：`FOREIGN`
- 大写宏：`GETSTRUCT`
- 大写宏：`GRANT`
- 大写宏：`IMMEDIATE`
- 大写宏：`INDEX`
- 大写宏：`INITIALLY`
- 大写宏：`KEY`
- 大写宏：`LIKE`
- 大写宏：`NIL`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`OBJECT_TABLE`
- 大写宏：`PRIMARY`
- 大写宏：`SCHEMA`
- 大写宏：`SEQUENCE`
- 大写宏：`TABLE`
- 大写宏：`TEMP`
- 大写宏：`TRIGGER`
- 大写宏：`VALUES`
- 大写宏：`VIEW`
- 大写宏：`XXX`
- 外部类型：`AT_AddConstraint`
- 外部类型：`After`
- 外部类型：`All`
- 外部类型：`AlterTableCmd`
- 外部类型：`AlterTableStmt`
- 外部类型：`Any`
- 外部类型：`But`
- 外部类型：`Check`
- 外部类型：`Column`
- 外部类型：`ColumnDef`
- 外部类型：`Complain`
- 外部类型：`Constraint`
- 外部类型：`CreateSchemaCommand`
- 外部类型：`CreateSchemaStmtContext`
- 外部类型：`CreateSeqStmt`
- 外部类型：`CreateStmt`
- 外部类型：`CreateStmtContext`
- 外部类型：`CreateTrigStmt`
- 外部类型：`Do`
- 外部类型：`Flat`
- 外部类型：`Form_pg_type`
- 外部类型：`If`
- 外部类型：`Immediately`
- 外部类型：`IndexStmt`
- 外部类型：`It`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Make`
- 外部类型：`Most`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Other`
- 外部类型：`Otherwise`
- 外部类型：`ParseState`
- 外部类型：`PartitionBoundSpec`
- 外部类型：`Pull`
- 外部类型：`RangeVar`
- 外部类型：`Relation`
- 外部类型：`Remove`
- 外部类型：`Run`
- 外部类型：`Scan`
- 外部类型：`This`
- 外部类型：`Type`
- 外部类型：`ViewStmt`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-7f718cb90a` → 本草稿移入 `cases/defect/auto-postgres-7f718cb90a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
