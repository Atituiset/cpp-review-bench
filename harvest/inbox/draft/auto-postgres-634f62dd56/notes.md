# auto-postgres-634f62dd56

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#2db51901c48e44369f5e6f209a65983c206075da](https://github.com/postgres/postgres/commit/2db51901c48e44369f5e6f209a65983c206075da) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2db51901c48e44369f5e6f209a65983c206075da (https://github.com/postgres/postgres/commit/2db51901c48e44369f5e6f209a65983c206075da)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 224；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2db51901c48e44369f5e6f209a65983c206075da Change float{4,8}in_internal take const char * input argument :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -221,12 +221,12 @@ float4in(PG_FUNCTION_ARGS)
  * comments also apply here, except regarding use in geometric types.
  */
 float4
-float4in_internal(char *num, char **endptr_p,
+float4in_internal(const char *num, char **endptr_p,
 				  const char *type_name, const char *orig_string,
 				  struct Node *escontext)
 {
 	float		val;
-	char	   *endptr;
+	const char *endptr;
 
 	/*
 	 * endptr points to the first character _after_ the sequence we recognized
@@ -249,7 +249,7 @@ float4in_internal(char *num, char **endptr_p,
 						type_name, orig_string)));
 
 	errno = 0;
-	val = strtof(num, &endptr);
+	val = strtof(num, unconstify(char **, &endptr));
 
 	/* did we not see anything that looks like a double? */
 	if (endptr == num || errno != 0)
@@ -342,7 +342,7 @@ float4in_internal(char *num, char **endptr_p,
 
 	/* report stopping point if wanted, else complain if not end of string */
 	if (endptr_p)
-		*endptr_p = endptr;
+		*endptr_p = unconstify(char *, endptr);
 	else if (*endptr != '\0')
 		ereturn(escontext, 0,
 				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
@@ -428,17 +428,14 @@ float8in(PG_FUNCTION_ARGS)
  * If escontext points to an ErrorSaveContext node, that is filled instead
  * of throwing an error; the caller must check SOFT_ERROR_OCCURRED()
  * to detect errors.
- *
- * "num" could validly be declared "const char *", but that results in an
- * unreasonable amount of extra casting both here and in callers, so we don't.
  */
 float8
-float8in_internal(char *num, char **endptr_p,
+float8in_internal(const char *num, char **endptr_p,
 				  const char *type_name, const char *orig_string,
 				  struct Node *escontext)
 {
 	double		val;
-	char	   *endptr;
+	const char *endptr;
 
 	/* skip leading whitespace */
 	while (*num != '\0' && isspace((unsigned char) *num))
@@ -455,7 +452,7 @@ float8in_internal(char *num, char **endptr_p,
 						type_name, orig_string)));
 
 	errno = 0;
-	val = strtod(num, &endptr);
+	val = strtod(num, unconstify(char **, &endptr));
 
 	/* did we not see anything that looks like a double? */
 	if (endptr == num || errno != 0)
@@ -544,7 +541,7 @@ float8in_internal(char *num, char **endptr_p,
 
 	/* report stopping point if wanted, else complain if not end of string */
 	if (endptr_p)
-		*endptr_p = endptr;
+		*endptr_p = unconstify(char *, endptr);
 	else if (*endptr != '\0')
 		ereturn(escontext, 0,
 				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereturn`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`get_float8_infinity`
- 外部函数：`get_float8_nan`
- 外部函数：`isspace`
- 外部函数：`pg_strncasecmp`
- 外部函数：`pstrdup`
- 外部函数：`strtof`
- 外部函数：`these`
- 大写宏：`C99`
- 大写宏：`ERANGE`
- 大写宏：`ERRCODE_INVALID_TEXT_REPRESENTATION`
- 大写宏：`ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE`
- 大写宏：`HUGE_VAL`
- 大写宏：`SOFT_ERROR_OCCURRED`
- 外部类型：`Check`
- 外部类型：`ErrorSaveContext`
- 外部类型：`If`
- 外部类型：`Inf`
- 外部类型：`Infinity`
- 外部类型：`NaN`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`On`
- 外部类型：`Some`
- 外部类型：`Therefore`
- 外部类型：`We`
- 外部类型：`You`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-634f62dd56` → 本草稿移入 `cases/defect/auto-postgres-634f62dd56/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
