# auto-postgres-57193bac86

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#245eb832f88631c986a2dadad2cbe94f57089fd6](https://github.com/postgres/postgres/commit/245eb832f88631c986a2dadad2cbe94f57089fd6) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 43 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #245eb832f88631c986a2dadad2cbe94f57089fd6 (https://github.com/postgres/postgres/commit/245eb832f88631c986a2dadad2cbe94f57089fd6)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 896；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 245eb832f88631c986a2dadad2cbe94f57089fd6 Introduce attribute versioning to prevent unsafe deforming :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -60,8 +60,11 @@
 #include "access/heaptoast.h"
 #include "access/sysattr.h"
 #include "access/tupdesc_details.h"
+#include "access/xlogdefs.h"
+#include "catalog/pg_type_d.h"
 #include "common/hashfn.h"
 #include "common/int.h"
+#include "utils/builtins.h"
 #include "utils/datum.h"
 #include "utils/expandeddatum.h"
 #include "utils/hsearch.h"
@@ -88,6 +91,11 @@
 #define COMPACT_ATTR_IS_PACKABLE(att) \
 	((att)->attlen == -1 && (att)->attispackable)
 
+
+/* GUC variable */
+int			composite_type_mismatch = COMPOSITE_TYPE_MISMATCH_ERROR;
+
+
 /*
  * Setup for caching pass-by-ref missing attributes in a way that survives
  * tupleDesc destruction.
@@ -790,6 +798,9 @@ expand_tuple(HeapTuple *targetHeapTuple,
 	Assert((targetHeapTuple && !targetMinimalTuple)
 		   || (!targetHeapTuple && targetMinimalTuple));
 
+	if (HeapTupleHeaderIsVersionedDatum(sourceTHeader))
+		heap_datum_check(sourceTHeader, tupleDesc);
+
 	Assert(sourceNatts < natts);
 
 	sourceNullLen = (hasNulls ? BITMAPLEN(sourceNatts) : 0);
@@ -893,11 +904,9 @@ expand_tuple(HeapTuple *targetHeapTuple,
 		targetTHeader->t_infomask = sourceTHeader->t_infomask;
 		targetTHeader->t_hoff = hoff;
 		HeapTupleHeaderSetNatts(targetTHeader, natts);
-		HeapTupleHeaderSetDatumLength(targetTHeader, len);
-		HeapTupleHeaderSetTypeId(targetTHeader, tupleDesc->tdtypeid);
-		HeapTupleHeaderSetTypMod(targetTHeader, tupleDesc->tdtypmod);
-		/* We also make sure that t_ctid is invalid unless explicitly set */
-		ItemPointerSetInvalid(&(targetTHeader->t_ctid));
+		/* invalidates t_ctid */
+		heap_tuple_set_datum(targetTHeader, tupleDesc, len);
+
 		if (targetNullLen > 0)
 			nullBits = (uint8 *) ((char *) (*targetHeapTuple)->t_data
 								  + offsetof(HeapTupleHeaderData, t_bits));
@@ -916,6 +925,10 @@ expand_tuple(HeapTuple *targetHeapTuple,
 		(*targetMinimalTuple)->t_infomask = sourceTHeader->t_infomask;
 		/* Same macro works for MinimalTuples */
 		HeapTupleHeaderSetNatts(*targetMinimalTuple, natts);
+
+		/* like HeapTupleHeaderClearDatum() */
+		(*targetMinimalTuple)->t_infomask &= ~HEAP_IS_DATUM;
+
 		if (targetNullLen > 0)
 			nullBits = (uint8 *) ((char *) *targetMinimalTuple
 								  + offsetof(MinimalTupleData, t_bits));
@@ -1023,6 +1036,11 @@ heap_copy_tuple_as_datum(HeapTuple tuple, TupleDesc tupleDesc)
 {
 	HeapTupleHeader td;
 
+	/*
+	 * TODO: Fetch the tupledesc for the datum tuple (if it's already one).
+	 * Without that it's somewhat easy to have this hide a mismatched tuple.
+	 */
+
 	/*
 	 * If the tuple contains any external TOAST pointers, we have to inline
 	 * those fields to meet the conventions for composite-type Datums.
@@ -1040,9 +1058,11 @@ heap_copy_tuple_as_datum(HeapTuple tuple, TupleDesc tupleDesc)
 	td = (HeapTupleHeader) palloc(tuple->t_len);
 	memcpy(td, tuple->t_data, tuple->t_len);
 
-	HeapTupleHeaderSetDatumLength(td, tuple->t_len);
-	HeapTupleHeaderSetTypeId(td, tupleDesc->tdtypeid);
-	HeapTupleHeaderSetTypMod(td, tupleDesc->tdtypmod);
+	/* if wrongly marked, set the type from scratch */
+	if (HeapTupleHeaderReallyIsDatum(td))
+		heap_datum_update_type(td, tupleDesc);
+	else
+		heap_tuple_set_datum(td, tupleDesc, tuple->t_len);
 
 	return PointerGetDatum(td);
 }
@@ -1116,15 +1136,21 @@ heap_form_tuple(TupleDesc tupleDescriptor,
 	ItemPointerSetInvalid(&(tuple->t_self));
 	tuple->t_tableOid = InvalidOid;
 
-	HeapTupleHeaderSetDatumLength(td, len);
-	HeapTupleHeaderSetTypeId(td, tupleDescriptor->tdtypeid);
-	HeapTupleHeaderSetTypMod(td, tupleDescriptor->tdtypmod);
-	/* We also make sure that t_ctid is invalid unless explicitly set */
-	ItemPointerSetInvalid(&(td->t_ctid));
-
+	/*
+	 * Mark the tuple as a datum-style tuple. This is safe even if we
+	 * eventually use it as a row type tuple, as e.g. inserting it into a
+	 * table will reset the relevant state.
+	 *
+	 * The reason we always set the datum fields is that there are a lot of
+	 * callers to heap_form_tuple() and the vast majority should have the
+	 * datum information. It's easier to c
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`att_nominal_alignby`
- 外部函数：`memcpy`
- 外部函数：`offsetof`
- 外部函数：`palloc`
- 外部函数：`store_att_byval`
- 外部函数：`strlen`
- 大写宏：`BITMAPLEN`
- 大写宏：`HEAP_HASEXTERNAL`
- 大写宏：`HEAP_HASNULL`
- 大写宏：`HEAP_HASVARWIDTH`
- 大写宏：`HIGHBIT`
- 大写宏：`NULL`
- 大写宏：`SET_VARSIZE_SHORT`
- 大写宏：`TOAST`
- 大写宏：`USE_ASSERT_CHECKING`
- 大写宏：`VARATT_CAN_MAKE_SHORT`
- 大写宏：`VARATT_CONVERTED_SHORT_SIZE`
- 大写宏：`VARATT_IS_EXTERNAL`
- 大写宏：`VARATT_IS_EXTERNAL_EXPANDED`
- 大写宏：`VARATT_IS_SHORT`
- 大写宏：`VARDATA`
- 大写宏：`VARSIZE`
- 大写宏：`VARSIZE_EXTERNAL`
- 大写宏：`VARSIZE_SHORT`
- 大写宏：`XXX`
- 外部类型：`CompactAttribute`
- 外部类型：`Datum`
- 外部类型：`Datums`
- 外部类型：`ExpandedObjectHeader`
- 外部类型：`HeapTupleHeader`
- 外部类型：`HeapTupleHeaderData`
- 外部类型：`If`
- 外部类型：`InvalidOid`
- 外部类型：`MinimalTupleData`
- 外部类型：`MinimalTuples`
- 外部类型：`Pointer`
- 外部类型：`Same`
- 外部类型：`Setup`
- 外部类型：`Size`
- 外部类型：`This`
- 外部类型：`TupleDesc`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-57193bac86` → 本草稿移入 `cases/defect/auto-postgres-57193bac86/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
