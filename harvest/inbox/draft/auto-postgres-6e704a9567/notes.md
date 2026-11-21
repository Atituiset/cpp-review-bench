# auto-postgres-6e704a9567

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#72146c7db15773ba0660c7c7e0284e3b1116ae2f](https://github.com/postgres/postgres/commit/72146c7db15773ba0660c7c7e0284e3b1116ae2f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #72146c7db15773ba0660c7c7e0284e3b1116ae2f (https://github.com/postgres/postgres/commit/72146c7db15773ba0660c7c7e0284e3b1116ae2f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 523；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 72146c7db15773ba0660c7c7e0284e3b1116ae2f Use an index scan in RemoveSubscriptionRel() when possible. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -520,34 +520,39 @@ void
 RemoveSubscriptionRel(Oid subid, Oid relid)
 {
 	Relation	rel;
-	TableScanDesc scan;
+	SysScanDesc scan;
 	ScanKeyData skey[2];
 	HeapTuple	tup;
 	int			nkeys = 0;
 
 	rel = table_open(SubscriptionRelRelationId, RowExclusiveLock);
 
-	if (OidIsValid(subid))
+	if (OidIsValid(relid))
 	{
 		ScanKeyInit(&skey[nkeys++],
-					Anum_pg_subscription_rel_srsubid,
+					Anum_pg_subscription_rel_srrelid,
 					BTEqualStrategyNumber,
 					F_OIDEQ,
-					ObjectIdGetDatum(subid));
+					ObjectIdGetDatum(relid));
 	}
 
-	if (OidIsValid(relid))
+	if (OidIsValid(subid))
 	{
 		ScanKeyInit(&skey[nkeys++],
-					Anum_pg_subscription_rel_srrelid,
+					Anum_pg_subscription_rel_srsubid,
 					BTEqualStrategyNumber,
 					F_OIDEQ,
-					ObjectIdGetDatum(relid));
+					ObjectIdGetDatum(subid));
 	}
 
-	/* Do the search and delete what we found. */
-	scan = table_beginscan_catalog(rel, nkeys, skey);
-	while (HeapTupleIsValid(tup = heap_getnext(scan, ForwardScanDirection)))
+	/*
+	 * Do the search and delete what we found.  Use the (srrelid, srsubid)
+	 * index when relid is given; otherwise (DROP SUBSCRIPTION) there is no
+	 * suitable index, so fall back to a heap scan.
+	 */
+	scan = systable_beginscan(rel, SubscriptionRelSrrelidSrsubidIndexId,
+							  OidIsValid(relid), NULL, nkeys, skey);
+	while (HeapTupleIsValid(tup = systable_getnext(scan)))
 	{
 		Form_pg_subscription_rel subrel;
 
@@ -584,7 +589,7 @@ RemoveSubscriptionRel(Oid subid, Oid relid)
 
 		CatalogTupleDelete(rel, &tup->t_self);
 	}
-	table_endscan(scan);
+	systable_endscan(scan);
 
 	table_close(rel, RowExclusiveLock);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`heap_getnext`
- 外部函数：`table_beginscan_catalog`
- 外部函数：`table_close`
- 外部函数：`table_endscan`
- 外部函数：`table_open`
- 大写宏：`F_OIDEQ`
- 外部类型：`Anum_pg_subscription_rel_srrelid`
- 外部类型：`Anum_pg_subscription_rel_srsubid`
- 外部类型：`BTEqualStrategyNumber`
- 外部类型：`Do`
- 外部类型：`Form_pg_subscription_rel`
- 外部类型：`ForwardScanDirection`
- 外部类型：`HeapTuple`
- 外部类型：`Oid`
- 外部类型：`Relation`
- 外部类型：`RowExclusiveLock`
- 外部类型：`ScanKeyData`
- 外部类型：`SubscriptionRelRelationId`
- 外部类型：`TableScanDesc`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-6e704a9567` → 本草稿移入 `cases/defect/auto-postgres-6e704a9567/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
