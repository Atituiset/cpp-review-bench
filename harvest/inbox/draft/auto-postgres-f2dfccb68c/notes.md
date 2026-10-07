# auto-postgres-f2dfccb68c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#e40b509c08f34e245bfab4124e24297ddaf4d535](https://github.com/postgres/postgres/commit/e40b509c08f34e245bfab4124e24297ddaf4d535) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e40b509c08f34e245bfab4124e24297ddaf4d535 (https://github.com/postgres/postgres/commit/e40b509c08f34e245bfab4124e24297ddaf4d535)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2976；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e40b509c08f34e245bfab4124e24297ddaf4d535 Concurrent REPACK: Clear out dropped-column values from all tuples :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2819,7 +2819,7 @@ apply_concurrent_changes(BufFile *file, ChangeContext *chgcxt)
 			/*
 			 * Adjust spilled_tuple so that it can be used as the new tuple in
 			 * the update that we're about to replay.  This fixes TOAST
-			 * pointers as well as remove useless values from dropped columns.
+			 * pointers.
 			 */
 			prepare_concurrent_update(spilled_tuple, ondisk_tuple);
 
@@ -2948,6 +2948,7 @@ restore_tuple(BufFile *file, Relation relation, TupleTableSlot *slot)
 {
 	uint32		t_len;
 	HeapTuple	tup;
+	TupleDesc	desc = slot->tts_tupleDescriptor;
 	int			natt_ext;
 
 	/* Read the tuple. */
@@ -2965,6 +2966,16 @@ restore_tuple(BufFile *file, Relation relation, TupleTableSlot *slot)
 	 */
 	ExecForceStoreHeapTuple(tup, slot, false);
 
+	/*
+	 * Dropped columns can still have values in some tuples.  Null them out.
+	 * This saves space, and the new heap might not even have a TOAST table.
+	 */
+	for (int i = 0; i < desc->natts; i++)
+	{
+		if (TupleDescCompactAttr(desc, i)->attisdropped)
+			slot->tts_isnull[i] = true;
+	}
+
 	/*
 	 * Next, read any attributes we stored separately into the tts_values
 	 * array elements expecting them, if any.  This matches
@@ -2973,8 +2984,6 @@ restore_tuple(BufFile *file, Relation relation, TupleTableSlot *slot)
 	BufFileReadExact(file, &natt_ext, sizeof(natt_ext));
 	if (natt_ext > 0)
 	{
-		TupleDesc	desc = slot->tts_tupleDescriptor;
-
 		for (int i = 0; i < desc->natts; i++)
 		{
 			CompactAttribute *attr = TupleDescCompactAttr(desc, i);
@@ -3017,13 +3026,10 @@ restore_tuple(BufFile *file, Relation relation, TupleTableSlot *slot)
  * update that we're about to replay.
  *
  * We perform the following critical change:
- * - Any EXTERNAL_ONDISK toast pointers so that it points to the corresponding
- *   toast value in 'src' (the transient table) instead.  The TOAST storage for
- *   'dest' is going to be dropped, so these values cannot be used any longer.
- *
- * We also apply the following optimization:
- * - If any columns are dropped but the slot still contains values, mark them
- *   as null to avoid uselessly wasting space in the new relation.
+ * - Update any EXTERNAL_ONDISK toast pointers so that they point to the
+ *   corresponding toast values in 'src' (the transient table) instead.
+ *   The TOAST storage for 'dest' is going to be dropped, so these values
+ *   cannot be used any longer.
  */
 static void
 prepare_concurrent_update(TupleTableSlot *dest, TupleTableSlot *src)
@@ -3036,14 +3042,7 @@ prepare_concurrent_update(TupleTableSlot *dest, TupleTableSlot *src)
 		varlena    *varlena_dst;
 
 		if (attr->attisdropped)
-		{
-			if (!slot_attisnull(dest, i + 1))
-			{
-				slot_getsomeattrs(dest, i + 1);
-				dest->tts_isnull[i] = true;
-			}
 			continue;
-		}
 		if (attr->attlen != -1)
 			continue;
 		if (slot_attisnull(dest, i + 1))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`slot_attisnull`
- 外部函数：`slot_getsomeattrs`
- 大写宏：`EXTERNAL_ONDISK`
- 大写宏：`TOAST`
- 大写宏：`VARATT_IS_EXTERNAL_ONDISK`
- 外部类型：`Adjust`
- 外部类型：`Any`
- 外部类型：`CompactAttribute`
- 外部类型：`HeapTuple`
- 外部类型：`If`
- 外部类型：`Next`
- 外部类型：`Read`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f2dfccb68c` → 本草稿移入 `cases/defect/auto-postgres-f2dfccb68c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
