# auto-postgres-7fd9a487f6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#c230e14b44d2bc40d0d141861e4cbd3839843df9](https://github.com/postgres/postgres/commit/c230e14b44d2bc40d0d141861e4cbd3839843df9) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c230e14b44d2bc40d0d141861e4cbd3839843df9 (https://github.com/postgres/postgres/commit/c230e14b44d2bc40d0d141861e4cbd3839843df9)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 15（原始 PR diff 行 2021；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c230e14b44d2bc40d0d141861e4cbd3839843df9 Handle numeric overflow in numeric_minmax_multi_ops :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -69,6 +69,7 @@
 #include "catalog/pg_am.h"
 #include "catalog/pg_amop.h"
 #include "catalog/pg_type.h"
+#include "nodes/miscnodes.h"
 #include "utils/array.h"
 #include "utils/builtins.h"
 #include "utils/date.h"
@@ -77,6 +78,7 @@
 #include "utils/inet.h"
 #include "utils/lsyscache.h"
 #include "utils/memutils.h"
+#include "utils/numeric.h"
 #include "utils/pg_lsn.h"
 #include "utils/rel.h"
 #include "utils/syscache.h"
@@ -2018,19 +2020,45 @@ brin_minmax_multi_distance_tid(PG_FUNCTION_ARGS)
 Datum
 brin_minmax_multi_distance_numeric(PG_FUNCTION_ARGS)
 {
-	Datum		d;
-	Datum		a1 = PG_GETARG_DATUM(0);
-	Datum		a2 = PG_GETARG_DATUM(1);
+	Numeric		d;
+	Numeric		a1 = PG_GETARG_NUMERIC(0);
+	Numeric		a2 = PG_GETARG_NUMERIC(1);
+	float8		distance;
+	ErrorSaveContext escontext = {T_ErrorSaveContext};
+
+	/* if both values are NaN, then we consider them the same */
+	if (numeric_is_nan(a1) && numeric_is_nan(a2))
+		PG_RETURN_FLOAT8(0.0);
+
+	/* if one value is NaN, use infinite distance */
+	if (numeric_is_nan(a1) || numeric_is_nan(a2))
+		PG_RETURN_FLOAT8(get_float8_infinity());
 
 	/*
 	 * We know the values are range boundaries, but the range may be collapsed
 	 * (i.e. single points), with equal values.
 	 */
-	Assert(DatumGetBool(DirectFunctionCall2(numeric_le, a1, a2)));
+	Assert(DatumGetBool(DirectFunctionCall2(numeric_le, NumericGetDatum(a1),
+											NumericGetDatum(a2))));
+
+	/*
+	 * Calculate the float8 distance in an overflow-safe way. Return infinity
+	 * instead of error-ing out.
+	 */
+	d = numeric_sub_safe(a2, a1, (Node *) &escontext);
+	if (SOFT_ERROR_OCCURRED(&escontext))
+		distance = get_float8_infinity();
+	else
+	{
+		distance = DatumGetFloat8(DirectFunctionCall1(numeric_float8_no_overflow,
+													  NumericGetDatum(d)));
+		pfree(d);
+	}
 
-	d = DirectFunctionCall2(numeric_sub, a2, a1);	/* a2 - a1 */
+	PG_FREE_IF_COPY(a1, 0);
+	PG_FREE_IF_COPY(a2, 1);
 
-	PG_RETURN_DATUM(DirectFunctionCall1(numeric_float8, d));
+	PG_RETURN_FLOAT8(distance);
 }
 
 /*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`PG_FUNCTION_ARGS`
- 大写宏：`PG_GETARG_DATUM`
- 大写宏：`PG_RETURN_DATUM`
- 外部类型：`Datum`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-7fd9a487f6` → 本草稿移入 `cases/defect/auto-postgres-7fd9a487f6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
