# auto-postgres-952a344a33

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#4f89416c9dbc78da5679d32072315af56fbeb8ed](https://github.com/postgres/postgres/commit/4f89416c9dbc78da5679d32072315af56fbeb8ed) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4f89416c9dbc78da5679d32072315af56fbeb8ed (https://github.com/postgres/postgres/commit/4f89416c9dbc78da5679d32072315af56fbeb8ed)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4f89416c9dbc78da5679d32072315af56fbeb8ed Fix more issues with extended statistics on expressions :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -28,6 +28,8 @@ PG_FUNCTION_INFO_V1(int_custom_in);
 PG_FUNCTION_INFO_V1(int_custom_out);
 PG_FUNCTION_INFO_V1(int_custom_typanalyze_false);
 PG_FUNCTION_INFO_V1(int_custom_typanalyze_invalid);
+PG_FUNCTION_INFO_V1(int_custom_typanalyze_false_target);
+PG_FUNCTION_INFO_V1(int_custom_typanalyze_false_second);
 PG_FUNCTION_INFO_V1(int_custom_eq);
 PG_FUNCTION_INFO_V1(int_custom_ne);
 PG_FUNCTION_INFO_V1(int_custom_lt);
@@ -110,6 +112,41 @@ int_custom_typanalyze_invalid(PG_FUNCTION_ARGS)
 	PG_RETURN_BOOL(true);
 }
 
+/*
+ * int_custom_typanalyze_false_target
+ *
+ * Returns false with a stats target.
+ */
+Datum
+int_custom_typanalyze_false_target(PG_FUNCTION_ARGS)
+{
+	VacAttrStats *stats = (VacAttrStats *) PG_GETARG_POINTER(0);
+
+	if (stats->attstattarget >= 0)
+		PG_RETURN_BOOL(false);
+
+	PG_RETURN_BOOL(std_typanalyze(stats));
+}
+
+/*
+ * int_custom_typanalyze_false_second
+ *
+ * Returns false on the second call with a stats target.
+ */
+Datum
+int_custom_typanalyze_false_second(PG_FUNCTION_ARGS)
+{
+	static int	ncalls = 0;
+	VacAttrStats *stats = (VacAttrStats *) PG_GETARG_POINTER(0);
+
+	if (stats->attstattarget < 0)
+		ncalls = 0;
+	else if (++ncalls > 1)
+		PG_RETURN_BOOL(false);
+
+	PG_RETURN_BOOL(std_typanalyze(stats));
+}
+
 /*
  * Comparison functions for int_custom type
  */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`palloc`
- 外部函数：`pg_ltoa`
- 大写宏：`PG_FUNCTION_ARGS`
- 大写宏：`PG_FUNCTION_INFO_V1`
- 大写宏：`PG_GETARG_INT32`
- 大写宏：`PG_GETARG_POINTER`
- 大写宏：`PG_RETURN_BOOL`
- 大写宏：`PG_RETURN_CSTRING`
- 外部类型：`AnalyzeAttrFetchFunc`
- 外部类型：`Buggy`
- 外部类型：`Comparison`
- 外部类型：`Datum`
- 外部类型：`If`
- 外部类型：`Set`
- 外部类型：`VacAttrStats`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-952a344a33` → 本草稿移入 `cases/defect/auto-postgres-952a344a33/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
