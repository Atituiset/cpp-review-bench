# auto-postgres-607ab17d89

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d06ac896faa7ff14e6eca5c9bc27a34818adf634](https://github.com/postgres/postgres/commit/d06ac896faa7ff14e6eca5c9bc27a34818adf634) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d06ac896faa7ff14e6eca5c9bc27a34818adf634 (https://github.com/postgres/postgres/commit/d06ac896faa7ff14e6eca5c9bc27a34818adf634)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 15（原始 PR diff 行 1875；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d06ac896faa7ff14e6eca5c9bc27a34818adf634 Handle numeric overflow when handling range values :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -44,7 +44,9 @@
 #include "port/pg_bitutils.h"
 #include "utils/builtins.h"
 #include "utils/date.h"
+#include "utils/float.h"
 #include "utils/lsyscache.h"
+#include "utils/numeric.h"
 #include "utils/rangetypes.h"
 #include "utils/sortsupport.h"
 #include "utils/timestamp.h"
@@ -1847,7 +1849,6 @@ daterange_canonical(PG_FUNCTION_ARGS)
  *
  * Note that subtype_diff does return the difference, not the absolute value
  * of the difference, and it must take care to avoid overflow.
- * (numrange_subdiff is at some risk there ...)
  *----------------------------------------------------------
  */
 
@@ -1872,15 +1873,37 @@ int8range_subdiff(PG_FUNCTION_ARGS)
 Datum
 numrange_subdiff(PG_FUNCTION_ARGS)
 {
-	Datum		v1 = PG_GETARG_DATUM(0);
-	Datum		v2 = PG_GETARG_DATUM(1);
-	Datum		numresult;
+	Numeric		v1 = PG_GETARG_NUMERIC(0);
+	Numeric		v2 = PG_GETARG_NUMERIC(1);
+	Numeric		numresult;
 	float8		floatresult;
+	ErrorSaveContext escontext = {T_ErrorSaveContext};
 
-	numresult = DirectFunctionCall2(numeric_sub, v1, v2);
+	/*
+	 * The difference can exceed numeric's range, or be unrepresentable in
+	 * float8.  None of the callers is prepared to handle an error, so
+	 * saturate to infinity (or round to zero) instead.
+	 */
+	numresult = numeric_sub_safe(v1, v2, (Node *) &escontext);
+	if (SOFT_ERROR_OCCURRED(&escontext))
+	{
+		/* The values v1 and v2 may be in arbitrary order. */
+		if (DatumGetBool(DirectFunctionCall2(numeric_lt, NumericGetDatum(v1),
+											 NumericGetDatum(v2))))
+			floatresult = -get_float8_infinity();
+		else
+			floatresult = get_float8_infinity();
+	}
+	else
+	{
+		floatresult
+			= DatumGetFloat8(DirectFunctionCall1(numeric_float8_no_overflow,
+												 NumericGetDatum(numresult)));
+		pfree(numresult);
+	}
 
-	floatresult = DatumGetFloat8(DirectFunctionCall1(numeric_float8,
-													 numresult));
+	PG_FREE_IF_COPY(v1, 0);
+	PG_FREE_IF_COPY(v2, 1);
 
 	PG_RETURN_FLOAT8(floatresult);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`PG_FUNCTION_ARGS`
- 大写宏：`PG_GETARG_DATUM`
- 大写宏：`PG_RETURN_FLOAT8`
- 外部类型：`Datum`
- 外部类型：`Note`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-607ab17d89` → 本草稿移入 `cases/defect/auto-postgres-607ab17d89/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
