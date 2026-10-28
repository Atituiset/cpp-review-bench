# auto-postgres-d3c17bf1bb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#27417270ab4e929aa3731e55945872206c425f75](https://github.com/postgres/postgres/commit/27417270ab4e929aa3731e55945872206c425f75) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 49 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #27417270ab4e929aa3731e55945872206c425f75 (https://github.com/postgres/postgres/commit/27417270ab4e929aa3731e55945872206c425f75)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2329；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 27417270ab4e929aa3731e55945872206c425f75 Fix stale pg_index flags in the relcache for TOAST indexes :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2319,14 +2319,15 @@ RelationReloadIndexInfo(Relation relation)
 	RelationInitPhysicalAddr(relation);
 
 	/*
-	 * For a non-system index, there are fields of the pg_index row that are
-	 * allowed to change, so re-read that row and update the relcache entry.
-	 * Most of the info derived from pg_index (such as support function lookup
-	 * info) cannot change, and indeed the whole point of this routine is to
-	 * update the relcache entry without clobbering that data; so wholesale
+	 * For a non-catalog index (this includes the TOAST index of a user
+	 * table), there are fields of the pg_index row that are allowed to
+	 * change, so re-read that row and update the relcache entry.  Most of the
+	 * info derived from pg_index (such as support function lookup info)
+	 * cannot change, and indeed the whole point of this routine is to update
+	 * the relcache entry without clobbering that data; so wholesale
 	 * replacement is not appropriate.
 	 */
-	if (!IsSystemRelation(relation))
+	if (!IsCatalogRelation(relation))
 	{
 		HeapTuple	tuple;
 		Form_pg_index index;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`entries`
- 外部函数：`heap_copytuple`
- 外部函数：`heap_freetuple`
- 外部函数：`pg_index`
- 外部函数：`systable_beginscan`
- 外部函数：`systable_endscan`
- 外部函数：`systable_getnext`
- 外部函数：`table_close`
- 外部函数：`table_open`
- 大写宏：`ERROR`
- 大写宏：`FATAL`
- 大写宏：`F_OIDEQ`
- 大写宏：`GETSTRUCT`
- 大写宏：`GLOBALTABLESPACE_OID`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`RELKIND_HAS_STORAGE`
- 外部类型：`AccessShareLock`
- 外部类型：`Anum_pg_class_oid`
- 外部类型：`BTEqualStrategyNumber`
- 外部类型：`ClassOidIndexId`
- 外部类型：`Consult`
- 外部类型：`Even`
- 外部类型：`For`
- 外部类型：`Force`
- 外部类型：`Form_pg_class`
- 外部类型：`Form_pg_index`
- 外部类型：`HeapTuple`
- 外部类型：`If`
- 外部类型：`InvalidOid`
- 外部类型：`InvalidSubTransactionId`
- 外部类型：`Most`
- 外部类型：`Must`
- 外部类型：`MyDatabaseId`
- 外部类型：`MyDatabaseTableSpace`
- 外部类型：`No`
- 外部类型：`Oid`
- 外部类型：`Open`
- 外部类型：`RelFileNumber`
- 外部类型：`Relation`
- 外部类型：`RelationRelationId`
- 外部类型：`ScanKeyData`
- 外部类型：`Snapshot`
- 外部类型：`SubTransactionId`
- 外部类型：`SysScanDesc`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`TopSubTransactionId`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-d3c17bf1bb` → 本草稿移入 `cases/defect/auto-postgres-d3c17bf1bb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
