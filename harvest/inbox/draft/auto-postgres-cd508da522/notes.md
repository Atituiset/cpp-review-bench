# auto-postgres-cd508da522

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#35b349a530c61628f4b7a7968212e594297523c6](https://github.com/postgres/postgres/commit/35b349a530c61628f4b7a7968212e594297523c6) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 10 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #35b349a530c61628f4b7a7968212e594297523c6 (https://github.com/postgres/postgres/commit/35b349a530c61628f4b7a7968212e594297523c6)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 153；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 35b349a530c61628f4b7a7968212e594297523c6 Fix JSON_TABLE with sublinks in PASSING arguments :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -145,12 +145,18 @@ transformJsonTable(ParseState *pstate, JsonTable *jt)
 	/*
 	 * Transform JsonFuncExpr representing the top JSON_TABLE context_item and
 	 * pathspec into a dummy JSON_TABLE_OP JsonExpr.
+	 *
+	 * The raw PASSING arguments are transformed here and again for each
+	 * column that has a path (see transformJsonTableColumn()).  Since
+	 * transformExpr() modifies some raw nodes in place, such as replacing a
+	 * SubLink's subselect with its Query, each transformation must be given
+	 * its own copy, so that jt->passing stays untouched.
 	 */
 	jfe = makeNode(JsonFuncExpr);
 	jfe->op = JSON_TABLE_OP;
 	jfe->context_item = jt->context_item;
 	jfe->pathspec = (Node *) rootPathSpec->string;
-	jfe->passing = jt->passing;
+	jfe->passing = copyObject(jt->passing);
 	jfe->on_empty = NULL;
 	jfe->on_error = jt->on_error;
 	jfe->location = jt->location;
@@ -529,7 +535,8 @@ transformJsonTableColumn(JsonTableColumn *jtc, Node *contextItemExpr,
 		pathspec = makeStringConst(path.data, -1);
 	}
 	jfexpr->pathspec = pathspec;
-	jfexpr->passing = passingArgs;
+	/* Each column gets its own copy; see transformJsonTable(). */
+	jfexpr->passing = copyObject(passingArgs);
 	jfexpr->output = makeNode(JsonOutput);
 	jfexpr->output->typeName = jtc->typeName;
 	jfexpr->output->returning = makeNode(JsonReturning);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`makeNode`
- 外部函数：`makeStringConst`
- 大写宏：`JSON_TABLE`
- 大写宏：`JSON_TABLE_OP`
- 大写宏：`NULL`
- 外部类型：`JsonExpr`
- 外部类型：`JsonFuncExpr`
- 外部类型：`JsonOutput`
- 外部类型：`JsonReturning`
- 外部类型：`Node`
- 外部类型：`Transform`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-cd508da522` → 本草稿移入 `cases/defect/auto-postgres-cd508da522/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
