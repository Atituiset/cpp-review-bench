# auto-postgres-c1f3e9cc53

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b3dbde59376f66e063ceacc03b6d2102825de692](https://github.com/postgres/postgres/commit/b3dbde59376f66e063ceacc03b6d2102825de692) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 52 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b3dbde59376f66e063ceacc03b6d2102825de692 (https://github.com/postgres/postgres/commit/b3dbde59376f66e063ceacc03b6d2102825de692)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1014；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b3dbde59376f66e063ceacc03b6d2102825de692 Remove hash_freeze() :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -234,9 +234,6 @@ struct HTAB
 	char	   *tabname;		/* table name (for error messages) */
 	bool		isshared;		/* true if table is in shared memory */
 
-	/* freezing a shared table isn't allowed, so we can keep state here */
-	bool		frozen;			/* true = no more inserts allowed */
-
 	/* We keep local copies of these fixed values to reduce contention */
 	Size		keysize;		/* hash key length in bytes */
 
@@ -534,8 +531,6 @@ hash_create(const char *tabname, int64 nelem, const HASHCTL *info, int flags)
 				(errcode(ERRCODE_OUT_OF_MEMORY),
 				 errmsg("out of memory")));
 
-	hashp->frozen = false;
-
 	hdefault(hashp);
 
 	hctl = hashp->hctl;
@@ -928,11 +923,11 @@ hash_search_with_hash_value(HTAB *hashp,
 	if (action == HASH_ENTER || action == HASH_ENTER_NULL)
 	{
 		/*
-		 * Can't split if running in partitioned mode, nor if frozen, nor if
-		 * table is the subject of any active hash_seq_search scans.
+		 * Can't split if running in partitioned mode, nor if table is the
+		 * subject of any active hash_seq_search scans.
 		 */
 		if (hctl->freeList[0].nentries > (int64) hctl->max_bucket &&
-			!hctl->isfixed && !IS_PARTITIONED(hctl) && !hashp->frozen &&
+			!hctl->isfixed && !IS_PARTITIONED(hctl) &&
 			!has_seq_scans(hashp))
 			(void) expand_table(hashp);
 	}
@@ -1010,11 +1005,6 @@ hash_search_with_hash_value(HTAB *hashp,
 			if (currBucket != NULL)
 				return ELEMENTKEY(currBucket);
 
-			/* disallow inserts if frozen */
-			if (hashp->frozen)
-				elog(ERROR, "cannot insert into frozen hashtable \"%s\"",
-					 hashp->tabname);
-
 			currBucket = get_hash_entry(hashp, freelist_idx);
 			if (currBucket == NULL)
 			{
@@ -1096,11 +1086,6 @@ hash_update_hash_key(HTAB *hashp,
 	hctl->accesses++;
 #endif
 
-	/* disallow updates if frozen */
-	if (hashp->frozen)
-		elog(ERROR, "cannot update in frozen hashtable \"%s\"",
-			 hashp->tabname);
-
 	/*
 	 * Lookup the existing element using its saved hash value.  We need to do
 	 * this to be able to unlink it from its hash chain, but as a side benefit
@@ -1306,10 +1291,6 @@ hash_get_num_entries(HTAB *hashp)
  * if elements are added to the table while the scan is in progress, it is
  * unspecified whether they will be visited by the scan or not.
  *
- * NOTE: it is possible to use hash_seq_init/hash_seq_search without any
- * worry about hash_seq_term cleanup, if the hashtable is first locked against
- * further insertions by calling hash_freeze.
- *
  * NOTE: to use this with a partitioned hashtable, caller had better hold
  * at least shared lock on all partitions of the table throughout the scan!
  * We can cope with insertions or deletions by our own backend, but *not*
@@ -1322,8 +1303,7 @@ hash_seq_init(HASH_SEQ_STATUS *status, HTAB *hashp)
 	status->curBucket = 0;
 	status->curEntry = NULL;
 	status->hasHashvalue = false;
-	if (!hashp->frozen)
-		register_seq_scan(hashp);
+	register_seq_scan(hashp);
 }
 
 /*
@@ -1445,32 +1425,7 @@ hash_seq_search(HASH_SEQ_STATUS *status)
 void
 hash_seq_term(HASH_SEQ_STATUS *status)
 {
-	if (!status->hashp->frozen)
-		deregister_seq_scan(status->hashp);
-}
-
-/*
- * hash_freeze
- *			Freeze a hashtable against future insertions (deletions are
- *			still allowed)
- *
- * The reason for doing this is that by preventing any more bucket splits,
- * we no longer need to worry about registering hash_seq_search scans,
- * and thus caller need not be careful about ensuring hash_seq_term gets
- * called at the right times.
- *
- * Multiple calls to hash_freeze() are allowed, but you can't freeze a table
- * with active scans (since hash_seq_term would then do the wrong thing).
- */
-void
-hash_freeze(HTAB *hashp)
-{
-	if (hashp->isshared)
-		elog(ERROR, "cannot freeze shared hashtable \"%s\"", hashp->tabname);
-	if (!hashp->frozen && has_seq_scans(hashp))
-		elog(ERROR, "cannot freeze hashtable \"%s\" because it has active scans",
-			 hashp->tabname);
-	hashp->frozen = true;
+	deregister_seq_scan(status->hashp);
 }
 
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`element_alloc`
- 外部函数：`elog`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`expand_table`
- 外部函数：`has_seq_scans`
- 外部函数：`log2`
- 外部函数：`name`
- 大写宏：`ERRCODE_OUT_OF_MEMORY`
- 大写宏：`ERROR`
- 大写宏：`HASHBUCKET`
- 大写宏：`HASHELEMENT`
- 大写宏：`HASHHDR`
- 大写宏：`HASHSEGMENT`
- 大写宏：`HASH_ENTER`
- 大写宏：`HASH_ENTER_NULL`
- 大写宏：`HASH_STATISTICS`
- 大写宏：`HTAB`
- 大写宏：`MAXALIGN`
- 大写宏：`MCXT_ALLOC_NO_OOM`
- 大写宏：`MUST`
- 大写宏：`NOTE`
- 大写宏：`NO_MAX_DSIZE`
- 大写宏：`NULL`
- 外部类型：`Can`
- 外部类型：`Failing`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Lookup`
- 外部类型：`MemoryContext`
- 外部类型：`No`
- 外部类型：`Size`
- 外部类型：`There`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-c1f3e9cc53` → 本草稿移入 `cases/defect/auto-postgres-c1f3e9cc53/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
