# auto-postgres-8b7fa6603f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#ee7cfb99a34cfba1cffdd456dc269243ddc69f3b](https://github.com/postgres/postgres/commit/ee7cfb99a34cfba1cffdd456dc269243ddc69f3b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 20（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ee7cfb99a34cfba1cffdd456dc269243ddc69f3b (https://github.com/postgres/postgres/commit/ee7cfb99a34cfba1cffdd456dc269243ddc69f3b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 4834；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ee7cfb99a34cfba1cffdd456dc269243ddc69f3b SQL/JSON: Fix ON EMPTY/ERROR state leakage on NULL input :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4830,29 +4830,25 @@ ExecInitJsonExpr(JsonExpr *jsexpr, ExprState *state,
 		jsestate->args = lappend(jsestate->args, var);
 	}
 
-	/* Step for jsonpath evaluation; see ExecEvalJsonExprPath(). */
-	scratch->opcode = EEOP_JSONEXPR_PATH;
-	scratch->resvalue = resv;
-	scratch->resnull = resnull;
-	scratch->d.jsonexpr.jsestate = jsestate;
-	ExprEvalPushStep(state, scratch);
-
 	/*
-	 * Step to return NULL after jumping to skip the EEOP_JSONEXPR_PATH step
-	 * when either formatted_expr or pathspec is NULL.  Adjust jump target
-	 * addresses of JUMPs that we added above.
+	 * Adjust jump target addresses of JUMPs that we added above to point to
+	 * the EEOP_JSONEXPR_PATH step.  That step itself handles the case where
+	 * either formatted_expr or pathspec is NULL by returning NULL, which
+	 * ensures that JsonExprState's empty/error flags are reset for every row
+	 * before the ON EMPTY / ON ERROR steps below look at them.
 	 */
 	foreach(lc, jumps_return_null)
 	{
 		ExprEvalStep *as = &state->steps[lfirst_int(lc)];
 
 		as->d.jump.jumpdone = state->steps_len;
 	}
-	scratch->opcode = EEOP_CONST;
+
+	/* Step for jsonpath evaluation; see ExecEvalJsonExprPath(). */
+	scratch->opcode = EEOP_JSONEXPR_PATH;
 	scratch->resvalue = resv;
 	scratch->resnull = resnull;
-	scratch->d.constval.value = (Datum) 0;
-	scratch->d.constval.isnull = true;
+	scratch->d.jsonexpr.jsestate = jsestate;
 	ExprEvalPushStep(state, scratch);
 
 	escontext = jsexpr->on_error->btype != JSON_BEHAVIOR_ERROR ?
@@ -4867,7 +4863,7 @@ ExecInitJsonExpr(JsonExpr *jsexpr, ExprState *state,
 
 	/*
 	 * Steps to coerce the result value computed by EEOP_JSONEXPR_PATH or the
-	 * NULL returned on NULL input as described above.
+	 * NULL that it returns on NULL input.
 	 */
 	jsestate->jump_eval_coercion = -1;
 	if (jsexpr->use_json_coercion)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`lappend`
- 外部函数：`lfirst_int`
- 外部函数：`memcpy`
- 外部函数：`palloc_array`
- 外部函数：`repalloc_array`
- 大写宏：`EEOP_CONST`
- 大写宏：`EEOP_JSONEXPR_PATH`
- 大写宏：`JSON_BEHAVIOR_ERROR`
- 大写宏：`NULL`
- 外部类型：`Adjust`
- 外部类型：`Datum`
- 外部类型：`ExprEvalStep`
- 外部类型：`ExprState`
- 外部类型：`JUMPs`
- 外部类型：`Step`
- 外部类型：`Steps`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-8b7fa6603f` → 本草稿移入 `cases/defect/auto-postgres-8b7fa6603f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
