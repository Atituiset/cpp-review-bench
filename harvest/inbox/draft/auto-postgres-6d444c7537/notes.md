# auto-postgres-6d444c7537

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#e5d25959cf8761c7dbdd7cfbb91338e94a40e8e2](https://github.com/postgres/postgres/commit/e5d25959cf8761c7dbdd7cfbb91338e94a40e8e2) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 77 |
| 编译错误数（gcc syntax-only） | 30（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e5d25959cf8761c7dbdd7cfbb91338e94a40e8e2 (https://github.com/postgres/postgres/commit/e5d25959cf8761c7dbdd7cfbb91338e94a40e8e2)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 195；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e5d25959cf8761c7dbdd7cfbb91338e94a40e8e2 Clear out values from dropped columns during REPACK (CONCURRENTLY) :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -192,8 +192,8 @@ static void apply_concurrent_update(Relation rel, TupleTableSlot *spilled_tuple,
 static void apply_concurrent_delete(Relation rel, TupleTableSlot *slot);
 static void restore_tuple(BufFile *file, Relation relation,
 						  TupleTableSlot *slot);
-static void adjust_toast_pointers(Relation relation, TupleTableSlot *dest,
-								  TupleTableSlot *src);
+static void prepare_concurrent_update(TupleTableSlot *dest,
+									  TupleTableSlot *src);
 static bool find_target_tuple(Relation rel, ChangeContext *chgcxt,
 							  TupleTableSlot *locator,
 							  TupleTableSlot *retrieved);
@@ -2817,13 +2817,11 @@ apply_concurrent_changes(BufFile *file, ChangeContext *chgcxt)
 				elog(ERROR, "could not find target tuple");
 
 			/*
-			 * If 'tup' contains TOAST pointers, they point to the old
-			 * relation's toast. Copy the corresponding TOAST pointers for the
-			 * new relation from the existing tuple. (The fact that we
-			 * received a TOAST pointer here implies that the attribute hasn't
-			 * changed.)
+			 * Adjust spilled_tuple so that it can be used as the new tuple in
+			 * the update that we're about to replay.  This fixes TOAST
+			 * pointers as well as remove useless values from dropped columns.
 			 */
-			adjust_toast_pointers(rel, spilled_tuple, ondisk_tuple);
+			prepare_concurrent_update(spilled_tuple, ondisk_tuple);
 
 			apply_concurrent_update(rel, spilled_tuple, ondisk_tuple, chgcxt);
 
@@ -3015,11 +3013,20 @@ restore_tuple(BufFile *file, Relation relation, TupleTableSlot *slot)
 }
 
 /*
- * Adjust 'dest' replacing any EXTERNAL_ONDISK toast pointers with the
- * corresponding ones from 'src'.
+ * Adjust the tuple in 'dest' so that it can be used as the NEW tuple in an
+ * update that we're about to replay.
+ *
+ * We perform the following critical change:
+ * - Any EXTERNAL_ONDISK toast pointers so that it points to the corresponding
+ *   toast value in 'src' (the transient table) instead.  The TOAST storage for
+ *   'dest' is going to be dropped, so these values cannot be used any longer.
+ *
+ * We also apply the following optimization:
+ * - If any columns are dropped but the slot still contains values, mark them
+ *   as null to avoid uselessly wasting space in the new relation.
  */
 static void
-adjust_toast_pointers(Relation relation, TupleTableSlot *dest, TupleTableSlot *src)
+prepare_concurrent_update(TupleTableSlot *dest, TupleTableSlot *src)
 {
 	TupleDesc	desc = dest->tts_tupleDescriptor;
 
@@ -3029,7 +3036,14 @@ adjust_toast_pointers(Relation relation, TupleTableSlot *dest, TupleTableSlot *s
 		varlena    *varlena_dst;
 
 		if (attr->attisdropped)
+		{
+			if (!slot_attisnull(dest, i + 1))
+			{
+				slot_getsomeattrs(dest, i + 1);
+				dest->tts_isnull[i] = true;
+			}
 			continue;
+		}
 		if (attr->attlen != -1)
 			continue;
 		if (slot_attisnull(dest, i + 1))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`find_target_tuple`
- 外部函数：`memcpy`
- 外部函数：`palloc`
- 外部函数：`pgstat_progress_incr_param`
- 外部函数：`separately`
- 外部函数：`slot_attisnull`
- 外部函数：`slot_getsomeattrs`
- 外部函数：`table_tuple_delete`
- 外部函数：`table_tuple_update`
- 大写宏：`DELETE`
- 大写宏：`EIIT_IS_UPDATE`
- 大写宏：`EIIT_ONLY_SUMMARIZING`
- 大写宏：`ERRCODE_T_R_SERIALIZATION_FAILURE`
- 大写宏：`ERROR`
- 大写宏：`EXTERNAL_ONDISK`
- 大写宏：`HEAPTUPLESIZE`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`PROGRESS_REPACK_HEAP_TUPLES_DELETED`
- 大写宏：`PROGRESS_REPACK_HEAP_TUPLES_UPDATED`
- 大写宏：`TABLE_DELETE_NO_LOGICAL`
- 大写宏：`TABLE_UPDATE_NO_LOGICAL`
- 大写宏：`TOAST`
- 大写宏：`UPDATE`
- 大写宏：`VARATT_IS_EXTERNAL_INDIRECT`
- 大写宏：`VARHDRSZ`
- 大写宏：`VARSIZE_ANY`
- 外部类型：`Adjust`
- 外部类型：`AttrNumber`
- 外部类型：`BufFile`
- 外部类型：`Carry`
- 外部类型：`ChangeContext`
- 外部类型：`CompactAttribute`
- 外部类型：`Copy`
- 外部类型：`Delete`
- 外部类型：`EState`
- 外部类型：`Existing`
- 外部类型：`HeapTuple`
- 外部类型：`HeapTupleHeader`
- 外部类型：`If`
- 外部类型：`InvalidSnapshot`
- 外部类型：`LockTupleMode`
- 外部类型：`Needed`
- 外部类型：`Next`
- 外部类型：`Put`
- 外部类型：`Read`
- 外部类型：`Relation`
- 外部类型：`ResultRelInfo`
- 外部类型：`ScanKey`
- 外部类型：`Sequential`
- 外部类型：`Size`
- 外部类型：`TM_FailureData`
- 外部类型：`TM_Ok`
- 外部类型：`TM_Result`
- 外部类型：`TU_None`
- 外部类型：`TU_Summarizing`
- 外部类型：`TU_UpdateIndexes`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`TupleDesc`
- 外部类型：`TupleTableSlot`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-6d444c7537` → 本草稿移入 `cases/defect/auto-postgres-6d444c7537/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
