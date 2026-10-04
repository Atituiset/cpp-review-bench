# auto-postgres-31816243fd

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#bfa1a69edd768078bc24d3930e069002167e7c9c](https://github.com/postgres/postgres/commit/bfa1a69edd768078bc24d3930e069002167e7c9c) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 87 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bfa1a69edd768078bc24d3930e069002167e7c9c (https://github.com/postgres/postgres/commit/bfa1a69edd768078bc24d3930e069002167e7c9c)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bfa1a69edd768078bc24d3930e069002167e7c9c Set hasSubLinks when expanding a whole-row join alias reference :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -884,8 +884,12 @@ flatten_join_alias_vars_mutator(Node *node,
 				if (IsA(newvar, Var))
 					((Var *) newvar)->location = var->location;
 				/* Recurse in case join input is itself a join */
-				/* (also takes care of setting inserted_sublink if needed) */
 				newvar = flatten_join_alias_vars_mutator(newvar, context);
+
+				/* Detect if we are adding a sublink to query */
+				if (context->possible_sublink && !context->inserted_sublink)
+					context->inserted_sublink = checkExprHasSubLink(newvar);
+
 				fields = lappend(fields, newvar);
 				/* We need the names of non-dropped columns, too */
 				colnames = lappend(colnames, copyObject((Node *) lfirst(ln)));
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_member`
- 外部函数：`bms_add_members`
- 外部函数：`bms_difference`
- 外部函数：`bms_equal`
- 外部函数：`bms_join`
- 外部函数：`expression_tree_walker`
- 外部函数：`level`
- 外部函数：`lfirst`
- 外部函数：`list_length`
- 外部函数：`query_or_expression_tree_walker`
- 外部函数：`query_tree_walker`
- 外部函数：`rt_fetch`
- 大写宏：`JOIN`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`PHV`
- 大写宏：`RTE`
- 大写宏：`RTE_JOIN`
- 外部类型：`Another`
- 外部类型：`But`
- 外部类型：`CurrentOfExpr`
- 外部类型：`However`
- 外部类型：`Ideally`
- 外部类型：`If`
- 外部类型：`Ignore`
- 外部类型：`In`
- 外部类型：`InvalidAttrNumber`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Must`
- 外部类型：`No`
- 外部类型：`Node`
- 外部类型：`Normal`
- 外部类型：`PlaceHolderInfo`
- 外部类型：`PlaceHolderVar`
- 外部类型：`PlaceHolderVars`
- 外部类型：`PlannerInfo`
- 外部类型：`Query`
- 外部类型：`RangeTblEntry`
- 外部类型：`Recurse`
- 外部类型：`Relids`
- 外部类型：`RowExpr`
- 外部类型：`SubLink`
- 外部类型：`Translated`
- 外部类型：`Typically`
- 外部类型：`Var`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-31816243fd` → 本草稿移入 `cases/defect/auto-postgres-31816243fd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
