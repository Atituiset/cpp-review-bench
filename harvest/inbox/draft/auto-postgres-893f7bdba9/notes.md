# auto-postgres-893f7bdba9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d94964f14f39936d69c6b8d3fc5020f0fdae1617](https://github.com/postgres/postgres/commit/d94964f14f39936d69c6b8d3fc5020f0fdae1617) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 43（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d94964f14f39936d69c6b8d3fc5020f0fdae1617 (https://github.com/postgres/postgres/commit/d94964f14f39936d69c6b8d3fc5020f0fdae1617)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 4219；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d94964f14f39936d69c6b8d3fc5020f0fdae1617 Pin relations in phase 1 of REINDEX CONCURRENTLY :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3790,6 +3790,7 @@ ReindexRelationConcurrently(const ReindexStmt *stmt, Oid relationOid, const Rein
 	List	   *indexIds = NIL;
 	List	   *newIndexIds = NIL;
 	List	   *relationLocks = NIL;
+	List	   *phase1Rels = NIL;
 	List	   *lockTags = NIL;
 	ListCell   *lc,
 			   *lc2;
@@ -3915,6 +3916,10 @@ ReindexRelationConcurrently(const ReindexStmt *stmt, Oid relationOid, const Rein
 						indexIds = lappend(indexIds, idx);
 
 						MemoryContextSwitchTo(oldcontext);
+
+						/* Keep it opened until session locks are taken */
+						phase1Rels = lappend(phase1Rels, indexRelation);
+						continue;
 					}
 
 					index_close(indexRelation, NoLock);
@@ -3964,6 +3969,10 @@ ReindexRelationConcurrently(const ReindexStmt *stmt, Oid relationOid, const Rein
 							/* other fields set later */
 
 							MemoryContextSwitchTo(oldcontext);
+
+							/* Keep it opened until session locks are taken */
+							phase1Rels = lappend(phase1Rels, indexRelation);
+							continue;
 						}
 
 						index_close(indexRelation, NoLock);
@@ -4201,10 +4210,10 @@ ReindexRelationConcurrently(const ReindexStmt *stmt, Oid relationOid, const Rein
 		newIndexIds = lappend(newIndexIds, newidx);
 
 		/*
-		 * Save lockrelid to protect each relation from drop then close
-		 * relations. The lockrelid on parent relation is not taken here to
-		 * avoid multiple locks taken on the same relation, instead we rely on
-		 * parentRelationIds built earlier.
+		 * Save lockrelid to protect each relation from drop.  The lockrelid
+		 * on parent relation is not taken here to avoid multiple locks taken
+		 * on the same relation, instead we rely on parentRelationIds built
+		 * earlier.
 		 */
 		lockrelid = palloc_object(LockRelId);
 		*lockrelid = indexRel->rd_lockInfo.lockRelId;
@@ -4215,8 +4224,13 @@ ReindexRelationConcurrently(const ReindexStmt *stmt, Oid relationOid, const Rein
 
 		MemoryContextSwitchTo(oldcontext);
 
+		/*
+		 * Index expressions and predicates of the indexes processed in the
+		 * next iterations are evaluated in this loop, so keep the new index
+		 * opened until the session locks are taken.
+		 */
 		index_close(indexRel, NoLock);
-		index_close(newIndexRel, NoLock);
+		phase1Rels = lappend(phase1Rels, newIndexRel);
 
 		/* Roll back any GUC changes executed by index functions */
 		AtEOXact_GUC(false, save_nestlevel);
@@ -4279,6 +4293,10 @@ ReindexRelationConcurrently(const ReindexStmt *stmt, Oid relationOid, const Rein
 		LockRelationIdForSession(lockrelid, ShareUpdateExclusiveLock);
 	}
 
+	/* Session locks are taken, release the relations opened in phase 1 */
+	ReindexUnpinRelations(phase1Rels);
+	phase1Rels = NIL;
+
 	PopActiveSnapshot();
 	CommitTransactionCommand();
 	StartTransactionCommand();
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`index_close`
- 外部函数：`lappend`
- 外部函数：`palloc_object`
- 大写宏：`GUC`
- 大写宏：`NIL`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`LockRelId`
- 外部类型：`NoLock`
- 外部类型：`Roll`
- 外部类型：`Save`
- 外部类型：`ShareUpdateExclusiveLock`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-893f7bdba9` → 本草稿移入 `cases/defect/auto-postgres-893f7bdba9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
