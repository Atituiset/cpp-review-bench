# auto-postgres-dfcdf70abb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b7eee09eab738e085d6b62d35788f542dd233877](https://github.com/postgres/postgres/commit/b7eee09eab738e085d6b62d35788f542dd233877) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b7eee09eab738e085d6b62d35788f542dd233877 (https://github.com/postgres/postgres/commit/b7eee09eab738e085d6b62d35788f542dd233877)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b7eee09eab738e085d6b62d35788f542dd233877 Fix NOT IN to anti-join conversion with coerced sub-select outputs :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1478,7 +1478,8 @@ convert_ANY_sublink_to_join(PlannerInfo *root, SubLink *sublink,
 
 /*
  * sublink_testexpr_is_not_nullable: verify that testexpr of an ANY_SUBLINK
- * guarantees a non-null result, assuming the inner side is also non-null.
+ * guarantees a non-null result, assuming the sub-select's outputs are also
+ * non-null.
  *
  * To ensure the expression never returns NULL, we require both that the outer
  * expressions are provably non-nullable and that the operator itself is safe.
@@ -1494,6 +1495,9 @@ convert_ANY_sublink_to_join(PlannerInfo *root, SubLink *sublink,
  * multi-column ordering checks.  In all cases, we validate the operators and
  * the outer expressions.
  *
+ * We also require each inner operand to be a bare (or relabeled) sublink
+ * Param, since a cast function on it could return NULL for a non-null input.
+ *
  * It is acceptable for this check not to be exhaustive.  We can err on the
  * side of conservatism: if we're not sure, it's okay to return FALSE.
  */
@@ -1502,6 +1506,7 @@ sublink_testexpr_is_not_nullable(PlannerInfo *root, SubLink *sublink)
 {
 	Node	   *testexpr = sublink->testexpr;
 	List	   *outer_exprs = NIL;
+	List	   *inner_exprs = NIL;
 
 	/* Punt if sublink is not in the expected format */
 	if (sublink->subLinkType != ANY_SUBLINK || testexpr == NULL)
@@ -1525,6 +1530,7 @@ sublink_testexpr_is_not_nullable(PlannerInfo *root, SubLink *sublink)
 			return false;
 
 		outer_exprs = lappend(outer_exprs, linitial(opexpr->args));
+		inner_exprs = lappend(inner_exprs, lsecond(opexpr->args));
 	}
 	else if (is_andclause(testexpr) || is_orclause(testexpr))
 	{
@@ -1545,6 +1551,7 @@ sublink_testexpr_is_not_nullable(PlannerInfo *root, SubLink *sublink)
 				return false;
 
 			outer_exprs = lappend(outer_exprs, linitial(opexpr->args));
+			inner_exprs = lappend(inner_exprs, lsecond(opexpr->args));
 		}
 	}
 	else if (IsA(testexpr, RowCompareExpr))
@@ -1560,13 +1567,25 @@ sublink_testexpr_is_not_nullable(PlannerInfo *root, SubLink *sublink)
 		}
 
 		outer_exprs = list_concat(outer_exprs, rcexpr->largs);
+		inner_exprs = list_concat(inner_exprs, rcexpr->rargs);
 	}
 	else
 	{
 		/* Punt if other node types */
 		return false;
 	}
 
+	/* Check that every inner operand is a possibly-relabeled sublink Param */
+	foreach_ptr(Expr, expr, inner_exprs)
+	{
+		while (IsA(expr, RelabelType))
+			expr = ((RelabelType *) expr)->arg;
+
+		if (!IsA(expr, Param) ||
+			((Param *) expr)->paramkind != PARAM_SUBLINK)
+			return false;
+	}
+
 	/*
 	 * Since the query hasn't yet been through expression preprocessing, we
 	 * must apply flatten_join_alias_vars to the outer expressions to avoid
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`is_andclause`
- 外部函数：`is_orclause`
- 外部函数：`lappend`
- 外部函数：`linitial`
- 外部函数：`list_concat`
- 大写宏：`ANY_SUBLINK`
- 大写宏：`FALSE`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 外部类型：`In`
- 外部类型：`It`
- 外部类型：`List`
- 外部类型：`Node`
- 外部类型：`Punt`
- 外部类型：`RowCompareExpr`
- 外部类型：`Since`
- 外部类型：`To`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-dfcdf70abb` → 本草稿移入 `cases/defect/auto-postgres-dfcdf70abb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
