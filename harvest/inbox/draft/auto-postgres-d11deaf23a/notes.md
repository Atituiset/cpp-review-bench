# auto-postgres-d11deaf23a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#060eebf4179b40d8eacd56f2e6670a531f8b0733](https://github.com/postgres/postgres/commit/060eebf4179b40d8eacd56f2e6670a531f8b0733) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 60 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #060eebf4179b40d8eacd56f2e6670a531f8b0733 (https://github.com/postgres/postgres/commit/060eebf4179b40d8eacd56f2e6670a531f8b0733)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 216；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 060eebf4179b40d8eacd56f2e6670a531f8b0733 Remove OldHeap argument from build_new_indexes() :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -213,7 +213,7 @@ static void rebuild_relation_finish_concurrent(Relation NewHeap, Relation OldHea
 											   Oid identIdx,
 											   TransactionId frozenXid,
 											   MultiXactId cutoffMulti);
-static List *build_new_indexes(Relation NewHeap, Relation OldHeap, List *OldIndexes);
+static List *build_new_indexes(Relation NewHeap, List *OldIndexes);
 static void copy_index_constraints(Relation old_index, Oid new_index_id,
 								   Oid new_heap_id);
 static void copy_attribute_defaults(Oid old_heap_oid, Oid new_heap_oid);
@@ -3462,7 +3462,7 @@ rebuild_relation_finish_concurrent(Relation NewHeap, Relation OldHeap,
 	 * are not necessarily dangerous, but can make user confused if the
 	 * changes they do get lost due to REPACK.)
 	 */
-	ind_oids_new = build_new_indexes(NewHeap, OldHeap, ind_oids_old);
+	ind_oids_new = build_new_indexes(NewHeap, ind_oids_old);
 
 	/*
 	 * The identity index in the new relation appears in the same relative
@@ -3632,17 +3632,17 @@ rebuild_relation_finish_concurrent(Relation NewHeap, Relation OldHeap,
 }
 
 /*
- * Build indexes on NewHeap according to those on OldHeap.
+ * Build indexes on NewHeap according to those on the old heap.
  *
- * OldIndexes is the list of index OIDs on OldHeap. The contained indexes end
- * up locked using ShareUpdateExclusiveLock.
+ * OldIndexes is the list of index OIDs on the old heap. The contained indexes
+ * end up locked using ShareUpdateExclusiveLock.
  *
  * A list of OIDs of the corresponding indexes created on NewHeap is
  * returned. The order of items does match, so we can use these arrays to swap
  * index storage.
  */
 static List *
-build_new_indexes(Relation NewHeap, Relation OldHeap, List *OldIndexes)
+build_new_indexes(Relation NewHeap, List *OldIndexes)
 {
 	List	   *result = NIL;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`heap_modify_tuple`
- 外部函数：`recordDependencyOn`
- 外部函数：`systable_beginscan`
- 外部函数：`systable_endscan`
- 外部函数：`systable_getnext`
- 外部函数：`table_close`
- 外部函数：`table_open`
- 大写宏：`ATTNUM`
- 大写宏：`DEPENDENCY_AUTO`
- 大写宏：`ERROR`
- 大写宏：`F_OIDEQ`
- 大写宏：`GETSTRUCT`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`REPACK`
- 外部类型：`Add`
- 外部类型：`Anum_pg_attrdef_adrelid`
- 外部类型：`Anum_pg_attrdef_oid`
- 外部类型：`Anum_pg_attribute_atthasdef`
- 外部类型：`Anum_pg_constraint_conindid`
- 外部类型：`Anum_pg_constraint_conrelid`
- 外部类型：`Anum_pg_constraint_oid`
- 外部类型：`AttrDefaultIndexId`
- 外部类型：`AttrDefaultOidIndexId`
- 外部类型：`AttrDefaultRelationId`
- 外部类型：`AttributeRelationId`
- 外部类型：`BTEqualStrategyNumber`
- 外部类型：`Build`
- 外部类型：`ConstraintOidIndexId`
- 外部类型：`ConstraintRelationId`
- 外部类型：`ConstraintRelidTypidNameIndexId`
- 外部类型：`Create`
- 外部类型：`Datum`
- 外部类型：`Form_pg_attrdef`
- 外部类型：`Form_pg_constraint`
- 外部类型：`HeapTuple`
- 外部类型：`Insert`
- 外部类型：`List`
- 外部类型：`MultiXactId`
- 外部类型：`Natts_pg_attrdef`
- 外部类型：`Natts_pg_attribute`
- 外部类型：`Natts_pg_constraint`
- 外部类型：`NewHeap`
- 外部类型：`OIDs`
- 外部类型：`ObjectAddress`
- 外部类型：`Oid`
- 外部类型：`OldHeap`
- 外部类型：`OldIndexes`
- 外部类型：`Relation`
- 外部类型：`RelationRelationId`
- 外部类型：`Retrieve`
- 外部类型：`RowExclusiveLock`
- 外部类型：`ScanKeyData`
- 外部类型：`Set`
- 外部类型：`ShareUpdateExclusiveLock`
- 外部类型：`SysScanDesc`
- 外部类型：`The`
- 外部类型：`TransactionId`
- 外部类型：`TupleDesc`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-d11deaf23a` → 本草稿移入 `cases/defect/auto-postgres-d11deaf23a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
