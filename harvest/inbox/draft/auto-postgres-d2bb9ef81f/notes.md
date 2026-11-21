# auto-postgres-d2bb9ef81f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#67c10344ddbd1073f1b7aafb09d1b196a48735a1](https://github.com/postgres/postgres/commit/67c10344ddbd1073f1b7aafb09d1b196a48735a1) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 153 |
| 编译错误数（gcc syntax-only） | 59（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #67c10344ddbd1073f1b7aafb09d1b196a48735a1 (https://github.com/postgres/postgres/commit/67c10344ddbd1073f1b7aafb09d1b196a48735a1)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 467；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 67c10344ddbd1073f1b7aafb09d1b196a48735a1 Always prune dead members in MultiXactIdExpand, to fix assertion :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -416,6 +416,7 @@ MultiXactIdExpand(MultiXactId multi, TransactionId xid, MultiXactStatus status)
 	int			nmembers;
 	int			i;
 	int			j;
+	bool		alreadyMember;
 
 	Assert(MultiXactIdIsValid(multi));
 	Assert(TransactionIdIsValid(xid));
@@ -453,22 +454,6 @@ MultiXactIdExpand(MultiXactId multi, TransactionId xid, MultiXactStatus status)
 		return newMulti;
 	}
 
-	/*
-	 * If the TransactionId is already a member of the MultiXactId with the
-	 * same status, just return the existing MultiXactId.
-	 */
-	for (i = 0; i < nmembers; i++)
-	{
-		if (TransactionIdEquals(members[i].xid, xid) &&
-			(members[i].status == status))
-		{
-			debug_elog4(DEBUG2, "Expand: %u is already a member of %u",
-						xid, multi);
-			pfree(members);
-			return multi;
-		}
-	}
-
 	/*
 	 * Determine which of the members of the MultiXactId are still of
 	 * interest. This is any running transaction, and also any transaction
@@ -477,33 +462,63 @@ MultiXactIdExpand(MultiXactId multi, TransactionId xid, MultiXactStatus status)
 	 * update Xid in a multixact would cause errors elsewhere.)
 	 *
 	 * Removing dead members is not just an optimization: freezing of tuples
-	 * whose Xmax are multis depends on this behavior.
+	 * whose Xmax are multis depends on this behavior.  This can also help the
+	 * caller to calculate a weaker overall lock level for this multixid.  If
+	 * e.g. the caller has already determined that the updating transaction is
+	 * not running, it would be surprising for the result to still contain a
+	 * member with MultiXactStatusUpdate.
 	 *
 	 * Note we have the same race condition here as above: j could be 0 at the
 	 * end of the loop.
 	 */
 	newMembers = palloc_array(MultiXactMember, nmembers + 1);
 
+	alreadyMember = false;
 	for (i = 0, j = 0; i < nmembers; i++)
 	{
-		if (TransactionIdIsInProgress(members[i].xid) ||
-			(ISUPDATE_from_mxstatus(members[i].status) &&
-			 TransactionIdDidCommit(members[i].xid)))
+		if (TransactionIdEquals(members[i].xid, xid) &&
+			(members[i].status == status))
+		{
+			alreadyMember = true;
+			debug_elog4(DEBUG2, "Expand: %u is already a member of %u",
+						xid, multi);
+			newMembers[j].xid = xid;
+			newMembers[j++].status = status;
+		}
+		else if (TransactionIdIsInProgress(members[i].xid) ||
+				 (ISUPDATE_from_mxstatus(members[i].status) &&
+				  TransactionIdDidCommit(members[i].xid)))
 		{
 			newMembers[j].xid = members[i].xid;
 			newMembers[j++].status = members[i].status;
 		}
 	}
 
-	newMembers[j].xid = xid;
-	newMembers[j++].status = status;
-	newMulti = MultiXactIdCreateFromMembers(j, newMembers);
+	/*
+	 * If the TransactionId is already a member of the MultiXactId with the
+	 * same status, and there were no dead members to remove, just return the
+	 * existing MultiXactId.  (The caching in MultiXactIdCreateFromMembers()
+	 * would detect this case too, but let's save the effort.)
+	 */
+	if (alreadyMember && j == nmembers)
+	{
+		newMulti = multi;
+		debug_elog3(DEBUG2, "Expand: returning original multi %u", multi);
+	}
+	else
+	{
+		if (!alreadyMember)
+		{
+			newMembers[j].xid = xid;
+			newMembers[j++].status = status;
+		}
+		newMulti = MultiXactIdCreateFromMembers(j, newMembers);
+		debug_elog3(DEBUG2, "Expand: returning new multi %u", newMulti);
+	}
 
 	pfree(members);
 	pfree(newMembers);
 
-	debug_elog3(DEBUG2, "Expand: returning new multi %u", newMulti);
-
 	return newMulti;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`debug_elog2`
- 外部函数：`debug_elog3`
- 外部函数：`debug_elog4`
- 外部函数：`elog`
- 外部函数：`mXactCacheGetBySet`
- 外部函数：`mxid_to_string`
- 外部函数：`palloc_array`
- 外部函数：`pfree`
- 外部函数：`pg_attribute_counted_by`
- 大写宏：`DEBUG2`
- 大写宏：`DELAY_CHKPT_START`
- 大写宏：`ERROR`
- 大写宏：`FLEXIBLE_ARRAY_MEMBER`
- 大写宏：`INJECTION_POINT_CACHED`
- 大写宏：`INJECTION_POINT_LOAD`
- 大写宏：`MXID`
- 大写宏：`NULL`
- 大写宏：`RM_MULTIXACT_ID`
- 大写宏：`START_CRIT_SECTION`
- 大写宏：`WAL`
- 大写宏：`XID`
- 大写宏：`XLOG`
- 大写宏：`XLOG_MULTIXACT_CREATE_ID`
- 大写宏：`XXX`
- 外部类型：`Assign`
- 外部类型：`But`
- 外部类型：`Create`
- 外部类型：`Determine`
- 外部类型：`During`
- 外部类型：`Expand`
- 外部类型：`FirstMultiXactId`
- 外部类型：`If`
- 外部类型：`InvalidMultiXactId`
- 外部类型：`Load`
- 外部类型：`MEMBERs`
- 外部类型：`Make`
- 外部类型：`MaxMultiXactId`
- 外部类型：`MultiXact`
- 外部类型：`MultiXactId`
- 外部类型：`MultiXactIdCreate`
- 外部类型：`MultiXactIdExpand`
- 外部类型：`MultiXactIdSetOldestMember`
- 外部类型：`MultiXactIds`
- 外部类型：`MultiXactMember`
- 外部类型：`MultiXactMemberSlruDesc`
- 外部类型：`MultiXactMemberSlruReadContext`
- 外部类型：`MultiXactOffset`
- 外部类型：`MultiXactOffsetSlruDesc`
- 外部类型：`MultiXacts`
- 外部类型：`MyProc`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`OFFSETs`
- 外部类型：`OldestMulti`
- 外部类型：`Otherwise`
- 外部类型：`Prevent`
- 外部类型：`Removing`
- 外部类型：`See`
- 外部类型：`SizeOfMultiXactCreate`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`TransactionId`
- 外部类型：`Verify`
- 外部类型：`We`
- 外部类型：`XLogRecData`
- 外部类型：`Xid`
- 外部类型：`Xlog`
- 外部类型：`Xmax`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-d2bb9ef81f` → 本草稿移入 `cases/defect/auto-postgres-d2bb9ef81f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
