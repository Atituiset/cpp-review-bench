# auto-postgres-31bb0e7b0d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1d45c5da3a0ad22b8e172da352ed25bd70a2253a](https://github.com/postgres/postgres/commit/1d45c5da3a0ad22b8e172da352ed25bd70a2253a) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 215 |
| 编译错误数（gcc syntax-only） | 69（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1d45c5da3a0ad22b8e172da352ed25bd70a2253a (https://github.com/postgres/postgres/commit/1d45c5da3a0ad22b8e172da352ed25bd70a2253a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 2245；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1d45c5da3a0ad22b8e172da352ed25bd70a2253a Unset SK_SEARCHARRAY when inequality arrays degenerate. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -548,6 +548,7 @@ _bt_start_array_keys(IndexScanDesc scan, ScanDirection dir)
 		ScanKey		skey = &so->keyData[array->scan_key];
 
 		Assert(skey->sk_flags & SK_SEARCHARRAY);
+		Assert(skey->sk_strategy == BTEqualStrategyNumber);
 
 		_bt_array_set_low_or_high(rel, skey, array,
 								  ScanDirectionIsForward(dir));
@@ -1196,18 +1197,17 @@ _bt_checkkeys(IndexScanDesc scan, BTReadPageState *pstate, bool arrayKeys,
 #endif
 
 	/*
-	 * Only one _bt_check_compare call is required in the common case where
-	 * there are no equality strategy array scan keys.  With array keys, we
-	 * can only accept _bt_check_compare's answer unreservedly when it set
-	 * pstate.continuescan=true.
+	 * Only one _bt_check_compare call is required when there are no array
+	 * scan keys.  Otherwise, we can only accept _bt_check_compare's answer
+	 * unreservedly when it set pstate.continuescan=true.
 	 */
 	if (!arrayKeys || pstate->continuescan)
 		return res;
 
 	/*
-	 * _bt_check_compare call set continuescan=false in the presence of
-	 * equality type array keys.  This could mean that the tuple is just past
-	 * the end of matches for the current array keys.
+	 * _bt_check_compare call set continuescan=false in the presence of array
+	 * keys.  This could mean that the tuple is just past the end of matches
+	 * for the current array keys.
 	 *
 	 * It's also possible that the scan is still _before_ the _start_ of
 	 * tuples matching the current set of array keys.  Check for that first.
@@ -1501,14 +1501,12 @@ _bt_check_compare(IndexScanDesc scan, ScanDirection dir,
 				*continuescan = false;
 
 			/*
-			 * If this is a non-required equality-type array key, the tuple
-			 * needs to be checked against every possible array key.  Handle
-			 * this by "advancing" the scan key's array to a matching value
-			 * (if we're successful then the tuple might match the qual).
+			 * If this is a non-required array key, the tuple needs to be
+			 * checked against every possible array key.  Handle this by
+			 * "advancing" the scan key's array to a matching value (if we're
+			 * successful then the tuple might match the qual).
 			 */
-			else if (advancenonrequired &&
-					 key->sk_strategy == BTEqualStrategyNumber &&
-					 (key->sk_flags & SK_SEARCHARRAY))
+			else if (advancenonrequired && (key->sk_flags & SK_SEARCHARRAY))
 				return _bt_advance_array_keys(scan, NULL, tuple, tupnatts,
 											  tupdesc, *ikey, false);
 
@@ -2242,16 +2240,15 @@ _bt_advance_array_keys(IndexScanDesc scan, BTReadPageState *pstate,
 		int32		result;
 		int			set_elem = 0;
 
-		if (cur->sk_strategy == BTEqualStrategyNumber)
+		if (cur->sk_flags & SK_SEARCHARRAY)
 		{
 			/* Manage array state */
-			if (cur->sk_flags & SK_SEARCHARRAY)
-			{
-				array = &so->arrayKeys[arrayidx++];
-				Assert(array->scan_key == ikey);
-			}
+			array = &so->arrayKeys[arrayidx++];
+
+			Assert(array->scan_key == ikey);
+			Assert(cur->sk_strategy == BTEqualStrategyNumber);
 		}
-		else
+		else if (cur->sk_strategy != BTEqualStrategyNumber)
 		{
 			/*
 			 * Are any inequalities required in the opposite direction only
@@ -3683,10 +3680,12 @@ _bt_verify_keys_with_arraykeys(IndexScanDesc scan)
 		ScanKey		cur = so->keyData + ikey;
 		BTArrayKeyInfo *array;
 
-		if (cur->sk_strategy != BTEqualStrategyNumber ||
-			!(cur->sk_flags & SK_SEARCHARRAY))
+		if (!(cur->sk_flags & SK_SEARCHARRAY))
 			continue;
 
+		if (cur->sk_strategy != BTEqualStrategyNumber)
+			return false;
+
 		array = &so->arrayKeys[arrayidx++];
 		if (array->scan_key != ikey)
 			return false;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_bt_parallel_primscan_schedule`
- 外部函数：`_bt_readpage`
- 外部函数：`ahead`
- 外部函数：`array`
- 外部函数：`arrays`
- 外部函数：`assertion`
- 外部函数：`comparison`
- 外部函数：`correctly`
- 外部函数：`datum`
- 外部函数：`datumCopy`
- 外部函数：`decrement`
- 外部函数：`element`
- 外部函数：`elog`
- 外部函数：`finaltup`
- 外部函数：`first`
- 外部函数：`forward`
- 外部函数：`increment`
- 外部函数：`index_getattr`
- 外部函数：`instead`
- 外部函数：`keys`
- 外部函数：`likely`
- 外部函数：`lowest`
- 外部函数：`next`
- 外部函数：`ongoing`
- 外部函数：`page`
- 外部函数：`pfree`
- 外部函数：`scheduling`
- 外部函数：`space`
- 外部函数：`strategy`
- 外部函数：`truncated`
- 外部函数：`unlikely`
- 外部函数：`unsatisfied`
- 大写宏：`ASC`
- 大写宏：`DESC`
- 大写宏：`ERROR`
- 大写宏：`FIRST`
- 大写宏：`INVERT_COMPARE_RESULT`
- 大写宏：`ISNULL`
- 大写宏：`LAST`
- 大写宏：`MAXVAL`
- 大写宏：`MINVAL`
- 大写宏：`NEXT`
- 大写宏：`NOT`
- 大写宏：`NOT_NULL`
- 大写宏：`NULL`
- 大写宏：`NULLS`
- 大写宏：`ORDER`
- 大写宏：`PG_USED_FOR_ASSERTS_ONLY`
- 大写宏：`PRIOR`
- 大写宏：`SAOP`
- 大写宏：`SK_BT_DESC`
- 大写宏：`SK_BT_MAXVAL`
- 大写宏：`SK_BT_MINVAL`
- 大写宏：`SK_BT_NEXT`
- 大写宏：`SK_BT_NULLS_FIRST`
- 大写宏：`SK_BT_PRIOR`
- 大写宏：`SK_BT_REQBKWD`
- 大写宏：`SK_BT_REQFWD`
- 大写宏：`SK_BT_SKIP`
- 大写宏：`SK_ISNULL`
- 大写宏：`SK_ROW_END`
- 大写宏：`SK_ROW_HEADER`
- 大写宏：`SK_ROW_MEMBER`
- 大写宏：`SK_SEARCHARRAY`
- 大写宏：`SK_SEARCHNOTNULL`
- 大写宏：`SK_SEARCHNULL`
- 大写宏：`USE_ASSERT_CHECKING`
- 大写宏：`WHERE`
- 外部类型：`Accept`
- 外部类型：`Actually`
- 外部类型：`Advance`
- 外部类型：`Again`
- 外部类型：`All`
- 外部类型：`Also`
- 外部类型：`Apply`
- 外部类型：`Are`
- 外部类型：`Array`
- 外部类型：`Ask`
- 外部类型：`Assert`
- 外部类型：`Assume`
- 外部类型：`BTArrayKeyInfo`
- 外部类型：`BTEqualStrategyNumber`
- 外部类型：`BTGreaterEqualStrategyNumber`
- 外部类型：`BTGreaterStrategyNumber`
- 外部类型：`BTLessEqualStrategyNumber`
- 外部类型：`BTLessStrategyNumber`
- 外部类型：`BTReadPageState`
- 外部类型：`BTScanOpaque`
- 外部类型：`Being`
- 外部类型：`Binary`
- 外部类型：`But`
- 外部类型：`By`
- 外部类型：`Caller`
- 外部类型：`Calls`
- 外部类型：`Cannot`
- 外部类型：`Check`
- 外部类型：`Clearing`
- 外部类型：`Compare`
- 外部类型：`Consider`
- 外部类型：`Continue`
- 外部类型：`Couldn`
- 外部类型：`Current`
- 外部类型：`Datum`
- 外部类型：`Deal`
- 外部类型：`Decrement`
- 外部类型：`Does`
- 外部类型：`Done`
- 外部类型：`Either`
- 外部类型：`End`
- 外部类型：`Evaluate`
- 外部类型：`Even`
- 外部类型：`Existing`
- 外部类型：`Final`
- 外部类型：`First`
- 外部类型：`FmgrInfo`
- 外部类型：`For`
- 外部类型：`ForwardScanDirection`
- 外部类型：`Free`
- 外部类型：`Give`
- 外部类型：`Groveling`
- 外部类型：`Handle`
- 外部类型：`Here`
- 外部类型：`Highest`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Inconclusive`
- 外部类型：`Increment`
- 外部类型：`IndexScanDesc`
- 外部类型：`IndexTuple`
- 外部类型：`Input`
- 外部类型：`Interpret`
- 外部类型：`InvalidAttrNumber`
- 外部类型：`It`
- 外部类型：`Just`
- 外部类型：`Keep`
- 外部类型：`Like`
- 外部类型：`Looks`
- 外部类型：`Loop`
- 外部类型：`Lowest`
- 外部类型：`Maintain`
- 外部类型：`Make`
- 外部类型：`Manage`
- 外部类型：`Might`
- 外部类型：`Must`
- 外部类型：`NULLs`
- 外部类型：`Needed`
- 外部类型：`Nope`
- 外部类型：`Note`
- 外部类型：`Nothing`
- 外部类型：`Notice`
- 外部类型：`OffsetNumber`
- 外部类型：`On`
- 外部类型：`Once`
- 外部类型：`Only`
- 外部类型：`Opclasses`
- 外部类型：`Optimistic`
- 外部类型：`Optimization`
- 外部类型：`Otherwise`
- 外部类型：`Our`
- 外部类型：`Output`
- 外部类型：`Override`
- 外部类型：`Page`
- 外部类型：`Per`
- 外部类型：`Perform`
- 外部类型：`Postcondition`
- 外部类型：`Precondition`
- 外部类型：`Private`
- 外部类型：`Proactively`
- 外部类型：`Recheck`
- 外部类型：`Relation`
- 外部类型：`Remember`
- 外部类型：`Repeated`
- 外部类型：`Requested`
- 外部类型：`Reset`
- 外部类型：`Restore`
- 外部类型：`Scalar`
- 外部类型：`ScanDirection`
- 外部类型：`ScanKey`
- 外部类型：`Scankey`
- 外部类型：`Schedule`
- 外部类型：`Search`
- 外部类型：`See`
- 外部类型：`Separately`
- 外部类型：`Set`
- 外部类型：`Setting`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`Some`
- 外部类型：`Specifically`
- 外部类型：`Start`
- 外部类型：`Stick`
- 外部类型：`Successfully`
- 外部类型：`Tell`
- 外部类型：`Terminate`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`These`
- 外部类型：`They`
- 外部类型：`This`
- 外部类型：`Though`
- 外部类型：`Track`
- 外部类型：`Truncated`
- 外部类型：`Tuple`
- 外部类型：`TupleDesc`
- 外部类型：`Under`
- 外部类型：`Unlike`
- 外部类型：`Use`
- 外部类型：`Verify`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`While`
- 外部类型：`With`
- 外部类型：`Without`
- 外部类型：`You`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-31bb0e7b0d` → 本草稿移入 `cases/defect/auto-postgres-31bb0e7b0d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
