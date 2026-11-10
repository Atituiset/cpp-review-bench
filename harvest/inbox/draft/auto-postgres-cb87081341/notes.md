# auto-postgres-cb87081341

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#17fc99f4e50c878d56de798e80ebcbb7a99e312c](https://github.com/postgres/postgres/commit/17fc99f4e50c878d56de798e80ebcbb7a99e312c) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 71 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #17fc99f4e50c878d56de798e80ebcbb7a99e312c (https://github.com/postgres/postgres/commit/17fc99f4e50c878d56de798e80ebcbb7a99e312c)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1375；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 17fc99f4e50c878d56de798e80ebcbb7a99e312c Add checks for cross-rowtype deforming and conversions :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -69,6 +69,7 @@
 #include "utils/expandeddatum.h"
 #include "utils/hsearch.h"
 #include "utils/memutils.h"
+#include "utils/typcache.h"
 
 
 /*
@@ -92,6 +93,9 @@
 	((att)->attlen == -1 && (att)->attispackable)
 
 
+static void heap_datum_check_coercion(HeapTupleHeader tup, TupleDesc targetDesc);
+
+
 /* GUC variable */
 int			composite_type_mismatch = COMPOSITE_TYPE_MISMATCH_ERROR;
 
@@ -1036,11 +1040,6 @@ heap_copy_tuple_as_datum(HeapTuple tuple, TupleDesc tupleDesc)
 {
 	HeapTupleHeader td;
 
-	/*
-	 * TODO: Fetch the tupledesc for the datum tuple (if it's already one).
-	 * Without that it's somewhat easy to have this hide a mismatched tuple.
-	 */
-
 	/*
 	 * If the tuple contains any external TOAST pointers, we have to inline
 	 * those fields to meet the conventions for composite-type Datums.
@@ -1366,19 +1365,155 @@ heap_tuple_set_datum(HeapTupleHeader tup, TupleDesc tupleDesc, uint32 len)
  * If the tuple already is a versioned datum, it is verified to be compatible
  * with tupleDesc first, as relabeling it would otherwise hide a
  * mismatch.
+ *
+ * Additionally, if the current type and the target type differ, the types are
+ * verified to be compatible (if possible).
  */
 void
 heap_datum_update_type(HeapTupleHeader tup, TupleDesc tupleDesc)
 {
 	Assert(HeapTupleHeaderReallyIsDatum(tup));
 
-	if (HeapTupleHeaderIsVersionedDatum(tup))
-		heap_datum_check(tup, tupleDesc);
+	heap_datum_check_coercion(tup, tupleDesc);
 
 	heap_tuple_set_datum(tup, tupleDesc,
 						 HeapTupleHeaderGetDatumLength(tup));
 }
 
+/*
+ * Verify that the datum tuple "tup" can be deformed with "deformingDesc",
+ * the descriptor the caller wants to use.
+ *
+ * "datumDesc" is the descriptor of the datum's own row type, against which
+ * the datum's version has already been checked (if it is versioned).  Only
+ * the attributes present in the tuple are compared, so this does not
+ * establish that the two descriptors are interchangeable for other tuples.
+ */
+static void
+verify_tupledesc_compatibility(HeapTupleHeader tup,
+							   TupleDesc datumDesc, TupleDesc deformingDesc)
+{
+	int			datum_natts = HeapTupleHeaderGetNatts(tup);
+	int			error_level =
+		composite_type_mismatch == COMPOSITE_TYPE_MISMATCH_WARN ?
+		WARNING : ERROR;
+
+	/*
+	 * Should never get here with a datumDesc that has insufficient
+	 * attributes.
+	 */
+	if (datumDesc->natts < datum_natts)
+	{
+		elog(error_level, "datum's descriptor has fewer (%d) attributes than tuple (%d)",
+			 datumDesc->natts, datum_natts);
+		return;
+	}
+
+	/*
+	 * We sometimes may get called with a deformingDesc that has extra
+	 * attributes for resjunk columns. That's safe iff the tuple itself does
+	 * not have those columns (the expected case). But we can't accept cases
+	 * where the datum has more attributes than deformingDesc.
+	 */
+	if (unlikely(deformingDesc->natts < datum_natts))
+	{
+		ereport(error_level,
+				errcode(ERRCODE_DATATYPE_MISMATCH),
+				errmsg("tuple descriptor has fewer (%d) attributes than tuple (%d)",
+					   deformingDesc->natts, datum_natts));
+		return;
+	}
+
+	/*
+	 * The message isn't translatable, due to the interpolated format strings.
+	 * That's OK, as this is just a defensive layer that shouldn't be reached.
+	 *
+	 * With composite_type_mismatch = warn, only the first mismatch is
+	 * reported.
+	 */
+#define check_attribute_aspect(aspect, format) \
+	if (unlikely(attr_datum->aspect != attr_deform->aspect)) \
+	{ \
+		ereport(error_level, \
+				errcode(ERRCODE_DATATYPE_MISMATCH), \
+				errmsg_internal("datum attribute \"%s\" at position %d does not match deforming descriptor's %s ("format" vs "format")", \
+								NameStr(attr_datum->attname), \
+								i + 1, \
+								CppAsString(aspect), \
+								attr_datum->aspect, attr_deform->aspect) \
+			); \
+		return; \
+	}
+
+	for (int i = 0; i < datum_natts; i++)
+	{
+		Form_pg_attribute attr_datum = TupleDescAttr(datumDesc, i);
+		Form_pg_attribute attr_deform = TupleDescAttr(deformingDesc, i);

```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errhint`
- 外部函数：`errmsg`
- 外部函数：`fewer`
- 外部函数：`format_type_be`
- 外部函数：`heap_tuple_set_datum`
- 外部函数：`later`
- 外部函数：`s`
- 外部函数：`unlikely`
- 外部函数：`versioning`
- 大写宏：`COMPOSITE_TYPE_MISMATCH_DISABLED`
- 大写宏：`COMPOSITE_TYPE_MISMATCH_ERROR`
- 大写宏：`COMPOSITE_TYPE_MISMATCH_WARN`
- 大写宏：`CONTEXT`
- 大写宏：`DETAIL`
- 大写宏：`ERRCODE_DATUM_VERSION_MISMATCH`
- 大写宏：`ERROR`
- 大写宏：`GUC`
- 大写宏：`LOG_SERVER_ONLY`
- 大写宏：`LSN_FORMAT_ARGS`
- 大写宏：`OID`
- 大写宏：`README`
- 大写宏：`RECORDOID`
- 大写宏：`TOAST`
- 大写宏：`TODO`
- 大写宏：`VERBOSITY`
- 大写宏：`WARNING`
- 外部类型：`Check`
- 外部类型：`Datums`
- 外部类型：`Fetch`
- 外部类型：`For`
- 外部类型：`HeapTupleHeader`
- 外部类型：`Hopefully`
- 外部类型：`If`
- 外部类型：`Iff`
- 外部类型：`InvalidXLogRecPtr`
- 外部类型：`Invalidating`
- 外部类型：`It`
- 外部类型：`Oid`
- 外部类型：`Putting`
- 外部类型：`Report`
- 外部类型：`See`
- 外部类型：`Stamp`
- 外部类型：`That`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`TupleDesc`
- 外部类型：`We`
- 外部类型：`Without`
- 外部类型：`XLogRecPtr`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-cb87081341` → 本草稿移入 `cases/defect/auto-postgres-cb87081341/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
