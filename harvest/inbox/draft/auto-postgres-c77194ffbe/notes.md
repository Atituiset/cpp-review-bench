# auto-postgres-c77194ffbe

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#15b3dc140d9109e37082454bc5041a9e6bddbd8e](https://github.com/postgres/postgres/commit/15b3dc140d9109e37082454bc5041a9e6bddbd8e) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 36（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15b3dc140d9109e37082454bc5041a9e6bddbd8e (https://github.com/postgres/postgres/commit/15b3dc140d9109e37082454bc5041a9e6bddbd8e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 14（原始 PR diff 行 226；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 15b3dc140d9109e37082454bc5041a9e6bddbd8e Fix overflow problems in btree_gist's penalty and distance functions. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -91,12 +91,18 @@ gbt_datekey_cmp(const void *a, const void *b, FmgrInfo *flinfo)
 static float8
 gdb_date_dist(const void *a, const void *b, FmgrInfo *flinfo)
 {
-	/* we assume the difference can't overflow */
-	Datum		diff = DirectFunctionCall2(date_mi,
-										   DateADTGetDatum(*((const DateADT *) a)),
-										   DateADTGetDatum(*((const DateADT *) b)));
-
-	return (float8) abs(DatumGetInt32(diff));
+	DateADT		da = *((const DateADT *) a);
+	DateADT		db = *((const DateADT *) b);
+
+	/*
+	 * Treat same-sign infinities as equal, as the float8 opclass does; but
+	 * any other case involving infinite inputs yields infinite distance.
+	 */
+	if (DATE_NOT_FINITE(da) || DATE_NOT_FINITE(db))
+		return (da == db) ? 0.0 : get_float8_infinity();
+
+	/* the difference of two finite dates cannot overflow int32 */
+	return (float8) abs(da - db);
 }
 
 
@@ -119,12 +125,20 @@ PG_FUNCTION_INFO_V1(date_dist);
 Datum
 date_dist(PG_FUNCTION_ARGS)
 {
-	/* we assume the difference can't overflow */
-	Datum		diff = DirectFunctionCall2(date_mi,
-										   PG_GETARG_DATUM(0),
-										   PG_GETARG_DATUM(1));
-
-	PG_RETURN_INT32(abs(DatumGetInt32(diff)));
+	DateADT		a = PG_GETARG_DATEADT(0);
+	DateADT		b = PG_GETARG_DATEADT(1);
+
+	/*
+	 * As in gdb_date_dist, same-sign infinities are equal.  For other cases
+	 * involving infinite inputs, int4 has no infinity, so use PG_INT32_MAX.
+	 * This is consistent with the ordering produced by gdb_date_dist, since
+	 * no difference of two finite dates reaches PG_INT32_MAX.
+	 */
+	if (DATE_NOT_FINITE(a) || DATE_NOT_FINITE(b))
+		PG_RETURN_INT32((a == b) ? 0 : PG_INT32_MAX);
+
+	/* the difference of two finite dates cannot overflow int32 */
+	PG_RETURN_INT32(abs(a - b));
 }
 
 
@@ -206,32 +220,17 @@ gbt_date_penalty(PG_FUNCTION_ARGS)
 	dateKEY    *origentry = (dateKEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(0))->key);
 	dateKEY    *newentry = (dateKEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(1))->key);
 	float	   *result = (float *) PG_GETARG_POINTER(2);
-	int32		diff,
-				res;
-
-	diff = DatumGetInt32(DirectFunctionCall2(date_mi,
-											 DateADTGetDatum(newentry->upper),
-											 DateADTGetDatum(origentry->upper)));
-
-	res = Max(diff, 0);
-
-	diff = DatumGetInt32(DirectFunctionCall2(date_mi,
-											 DateADTGetDatum(origentry->lower),
-											 DateADTGetDatum(newentry->lower)));
-
-	res += Max(diff, 0);
-
-	*result = 0.0;
 
-	if (res > 0)
-	{
-		diff = DatumGetInt32(DirectFunctionCall2(date_mi,
-												 DateADTGetDatum(origentry->upper),
-												 DateADTGetDatum(origentry->lower)));
-		*result += FLT_MIN;
-		*result += (float) (res / ((double) (res + diff)));
-		*result *= (FLT_MAX / (((GISTENTRY *) PG_GETARG_POINTER(0))->rel->rd_att->natts + 1));
-	}
+	/*
+	 * Don't use date_mi here: it rejects infinite dates.  Plain int32
+	 * subtraction won't do either: it could overflow for cases involving
+	 * infinite dates.  But we can use penalty_num: it computes in double,
+	 * where every int32 and every difference of two int32s is exact.  The
+	 * output for cases with unequal infinite dates will be finite but large,
+	 * which seems fine for penalty calculations.
+	 */
+	penalty_num(result, origentry->lower, origentry->upper,
+				newentry->lower, newentry->upper);
 
 	PG_RETURN_POINTER(result);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`abs`
- 大写宏：`FLT_MAX`
- 大写宏：`FLT_MIN`
- 大写宏：`GISTENTRY`
- 大写宏：`PG_FUNCTION_ARGS`
- 大写宏：`PG_GETARG_DATUM`
- 大写宏：`PG_GETARG_POINTER`
- 大写宏：`PG_RETURN_INT32`
- 大写宏：`PG_RETURN_POINTER`
- 外部类型：`DateADT`
- 外部类型：`Datum`
- 外部类型：`FmgrInfo`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-c77194ffbe` → 本草稿移入 `cases/defect/auto-postgres-c77194ffbe/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
