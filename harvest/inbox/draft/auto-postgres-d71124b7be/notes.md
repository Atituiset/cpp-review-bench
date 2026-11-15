# auto-postgres-d71124b7be

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#81b2179b492e3af891544813b7e22a42386b34d0](https://github.com/postgres/postgres/commit/81b2179b492e3af891544813b7e22a42386b34d0) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 139 |
| 编译错误数（gcc syntax-only） | 41（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #81b2179b492e3af891544813b7e22a42386b34d0 (https://github.com/postgres/postgres/commit/81b2179b492e3af891544813b7e22a42386b34d0)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 754；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 81b2179b492e3af891544813b7e22a42386b34d0 Remove unused tuple expansion and copying functions :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -739,296 +739,6 @@ heap_copytuple(HeapTuple tuple)
 	return newTuple;
 }
 
-/* ----------------
- *		heap_copytuple_with_tuple
- *
- *		copy a tuple into a caller-supplied HeapTuple management struct
- *
- * Note that after calling this function, the "dest" HeapTuple will not be
- * allocated as a single palloc() block (unlike with heap_copytuple()).
- * ----------------
- */
-void
-heap_copytuple_with_tuple(HeapTuple src, HeapTuple dest)
-{
-	if (!HeapTupleIsValid(src) || src->t_data == NULL)
-	{
-		dest->t_data = NULL;
-		return;
-	}
-
-	dest->t_len = src->t_len;
-	dest->t_self = src->t_self;
-	dest->t_tableOid = src->t_tableOid;
-	dest->t_data = (HeapTupleHeader) palloc(src->t_len);
-	memcpy(dest->t_data, src->t_data, src->t_len);
-}
-
-/*
- * Expand a tuple which has fewer attributes than required. For each attribute
- * not present in the sourceTuple, if there is a missing value that will be
- * used. Otherwise the attribute will be set to NULL.
- *
- * The source tuple must have fewer attributes than the required number.
- *
- * Only one of targetHeapTuple and targetMinimalTuple may be supplied. The
- * other argument must be NULL.
- */
-static void
-expand_tuple(HeapTuple *targetHeapTuple,
-			 MinimalTuple *targetMinimalTuple,
-			 HeapTuple sourceTuple,
-			 TupleDesc tupleDesc)
-{
-	AttrMissing *attrmiss = NULL;
-	int			attnum;
-	int			firstmissingnum;
-	bool		hasNulls = HeapTupleHasNulls(sourceTuple);
-	HeapTupleHeader targetTHeader;
-	HeapTupleHeader sourceTHeader = sourceTuple->t_data;
-	int			sourceNatts = HeapTupleHeaderGetNatts(sourceTHeader);
-	int			natts = tupleDesc->natts;
-	int			sourceNullLen;
-	int			targetNullLen;
-	Size		sourceDataLen = sourceTuple->t_len - sourceTHeader->t_hoff;
-	Size		targetDataLen;
-	Size		len;
-	int			hoff;
-	uint8	   *nullBits = NULL;
-	int			bitMask = 0;
-	char	   *targetData;
-	uint16	   *infoMask;
-
-	Assert((targetHeapTuple && !targetMinimalTuple)
-		   || (!targetHeapTuple && targetMinimalTuple));
-
-	if (HeapTupleHeaderIsVersionedDatum(sourceTHeader))
-		heap_datum_check(sourceTHeader, tupleDesc);
-
-	Assert(sourceNatts < natts);
-
-	sourceNullLen = (hasNulls ? BITMAPLEN(sourceNatts) : 0);
-
-	targetDataLen = sourceDataLen;
-
-	if (tupleDesc->constr &&
-		tupleDesc->constr->missing)
-	{
-		/*
-		 * If there are missing values we want to put them into the tuple.
-		 * Before that we have to compute the extra length for the values
-		 * array and the variable length data.
-		 */
-		attrmiss = tupleDesc->constr->missing;
-
-		/*
-		 * Find the first item in attrmiss for which we don't have a value in
-		 * the source. We can ignore all the missing entries before that.
-		 */
-		for (firstmissingnum = sourceNatts;
-			 firstmissingnum < natts;
-			 firstmissingnum++)
-		{
-			if (attrmiss[firstmissingnum].am_present)
-				break;
-			else
-				hasNulls = true;
-		}
-
-		/*
-		 * Now walk the missing attributes. If there is a missing value make
-		 * space for it. Otherwise, it's going to be NULL.
-		 */
-		for (attnum = firstmissingnum;
-			 attnum < natts;
-			 attnum++)
-		{
-			if (attrmiss[attnum].am_present)
-			{
-				CompactAttribute *att = TupleDescCompactAttr(tupleDesc, attnum);
-
-				targetDataLen = att_datum_alignby(targetDataLen,
-												  att->attalignby,
-												  att->attlen,
-												  attrmiss[attnum].am_value);
-
-				targetDataLen = att_addlength_datum(targetDataLen,
-													att->attlen,
-													attrmiss[attnum].am_value);
-
-				if (!AllocSizeIsValid(targetDataLen))
-					ereport(ERROR,
-							errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
-							errmsg("tuple is too big"));
-			}
-			else
-			{
-				/* no missing value, so it must be null */
-				hasNulls = true;
-			}
-		}
-	}							/* end if have missing values */
-	else
-	{
-		/*
-		 * If there are no missing values at all then NULLS must be allowed,
-		 * since some of the attributes are known to be absent.
-		 */
-		hasNulls = true;
-	}
-
-	len = 0;
-
-	if (hasNulls)
-	{
-		tar
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`att_datum_alignby`
- 外部函数：`att_nominal_alignby`
- 外部函数：`block`
- 外部函数：`heap_copytuple`
- 外部函数：`heap_copytuple_with_tuple`
- 外部函数：`heap_datum_check`
- 外部函数：`memcpy`
- 外部函数：`store_att_byval`
- 外部函数：`strlen`
- 大写宏：`BITMAPLEN`
- 大写宏：`HEAP_HASEXTERNAL`
- 大写宏：`HEAP_HASNULL`
- 大写宏：`HEAP_HASVARWIDTH`
- 大写宏：`HIGHBIT`
- 大写宏：`NULL`
- 大写宏：`SET_VARSIZE_SHORT`
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
- 外部类型：`AttrMissing`
- 外部类型：`Before`
- 外部类型：`CompactAttribute`
- 外部类型：`Datum`
- 外部类型：`Expand`
- 外部类型：`ExpandedObjectHeader`
- 外部类型：`Find`
- 外部类型：`For`
- 外部类型：`HeapTuple`
- 外部类型：`HeapTupleHeader`
- 外部类型：`If`
- 外部类型：`MinimalTuple`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Only`
- 外部类型：`Otherwise`
- 外部类型：`Pointer`
- 外部类型：`Size`
- 外部类型：`The`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-d71124b7be` → 本草稿移入 `cases/defect/auto-postgres-d71124b7be/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
