# auto-postgres-8d891bbd3a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d2adf339b06b8046d69be7974f9f7e490d2c9e84](https://github.com/postgres/postgres/commit/d2adf339b06b8046d69be7974f9f7e490d2c9e84) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 39 |
| 编译错误数（gcc syntax-only） | 26（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d2adf339b06b8046d69be7974f9f7e490d2c9e84 (https://github.com/postgres/postgres/commit/d2adf339b06b8046d69be7974f9f7e490d2c9e84)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 6363；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d2adf339b06b8046d69be7974f9f7e490d2c9e84 Add PG_RETURN_ARG macro and use it for sum(int2) and sum(int4) :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6357,22 +6357,20 @@ int2_sum(PG_FUNCTION_ARGS)
 	int64		oldsum;
 	int64		newval;
 
+	if (PG_ARGISNULL(1))
+	{
+		/* NULL value being aggregated, so return state unchanged */
+		PG_RETURN_ARG(0);
+	}
+
 	if (PG_ARGISNULL(0))
 	{
-		/* No non-null input seen so far... */
-		if (PG_ARGISNULL(1))
-			PG_RETURN_NULL();	/* still no non-null */
 		/* This is the first non-null input. */
-		newval = (int64) PG_GETARG_INT16(1);
-		PG_RETURN_INT64(newval);
+		PG_RETURN_INT64((int64) PG_GETARG_INT16(1));
 	}
 
 	oldsum = PG_GETARG_INT64(0);
 
-	/* Leave sum unchanged if new input is null. */
-	if (PG_ARGISNULL(1))
-		PG_RETURN_INT64(oldsum);
-
 	/* OK to do the addition. */
 	newval = oldsum + (int64) PG_GETARG_INT16(1);
 
@@ -6385,22 +6383,20 @@ int4_sum(PG_FUNCTION_ARGS)
 	int64		oldsum;
 	int64		newval;
 
+	if (PG_ARGISNULL(1))
+	{
+		/* NULL value being aggregated, so return state unchanged */
+		PG_RETURN_ARG(0);
+	}
+
 	if (PG_ARGISNULL(0))
 	{
-		/* No non-null input seen so far... */
-		if (PG_ARGISNULL(1))
-			PG_RETURN_NULL();	/* still no non-null */
 		/* This is the first non-null input. */
-		newval = (int64) PG_GETARG_INT32(1);
-		PG_RETURN_INT64(newval);
+		PG_RETURN_INT64((int64) PG_GETARG_INT32(1));
 	}
 
 	oldsum = PG_GETARG_INT64(0);
 
-	/* Leave sum unchanged if new input is null. */
-	if (PG_ARGISNULL(1))
-		PG_RETURN_INT64(oldsum);
-
 	/* OK to do the addition. */
 	newval = oldsum + (int64) PG_GETARG_INT32(1);
 
@@ -6424,12 +6420,6 @@ int8_sum(PG_FUNCTION_ARGS)
 		PG_RETURN_NUMERIC(int64_to_numeric(PG_GETARG_INT64(1)));
 	}
 
-	/*
-	 * Note that we cannot special-case the aggregate case here, as we do for
-	 * int2_sum and int4_sum: numeric is of variable size, so we cannot modify
-	 * our first parameter in-place.
-	 */
-
 	oldsum = PG_GETARG_NUMERIC(0);
 
 	/* Leave sum unchanged if new input is null. */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`digitbuf_alloc`
- 外部函数：`digitbuf_free`
- 外部函数：`dump_numeric`
- 外部函数：`elog`
- 外部函数：`init_var`
- 外部函数：`int64_to_numeric`
- 外部函数：`int64_to_numericvar`
- 外部函数：`make_result`
- 外部函数：`memset`
- 外部函数：`palloc`
- 外部函数：`palloc_array`
- 外部函数：`pfree`
- 大写宏：`ERROR`
- 大写宏：`NULL`
- 大写宏：`PG_ARGISNULL`
- 大写宏：`PG_GETARG_INT16`
- 大写宏：`PG_GETARG_INT32`
- 大写宏：`PG_GETARG_INT64`
- 大写宏：`PG_GETARG_NUMERIC`
- 大写宏：`PG_RETURN_INT64`
- 大写宏：`PG_RETURN_NULL`
- 大写宏：`PG_RETURN_NUMERIC`
- 大写宏：`SET_VARSIZE`
- 大写宏：`VARHDRSZ`
- 大写宏：`VARSIZE`
- 外部类型：`Assert`
- 外部类型：`Leave`
- 外部类型：`No`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`Numeric`
- 外部类型：`NumericVar`
- 外部类型：`Size`
- 外部类型：`This`
- 外部类型：`Verify`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-8d891bbd3a` → 本草稿移入 `cases/defect/auto-postgres-8d891bbd3a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
