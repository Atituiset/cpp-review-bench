# auto-postgres-f7c162fe3a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#debfb3c8a8c93e7bcc3638523b7ffc0846cc2db8](https://github.com/postgres/postgres/commit/debfb3c8a8c93e7bcc3638523b7ffc0846cc2db8) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #debfb3c8a8c93e7bcc3638523b7ffc0846cc2db8 (https://github.com/postgres/postgres/commit/debfb3c8a8c93e7bcc3638523b7ffc0846cc2db8)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT debfb3c8a8c93e7bcc3638523b7ffc0846cc2db8 postgres_fdw: Update some comments in functions for statistics import. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6209,10 +6209,10 @@ remattrmap_cmp(const void *v1, const void *v2)
 /*
  * Match local columns to result set rows.
  *
- * As the result set consists of the attribute stats for some/all of distinct
- * mapped remote columns in the RemoteAttributeMapping, every entry in it
- * should have at most one match in the result set; which is also ordered by
- * remote_attname, so we find such pairs by doing a merge join.
+ * As the result set consists of the attribute stats for distinct mapped
+ * remote columns, every entry in the RemoteAttributeMapping should have at
+ * most one match in the result set; which is also ordered by remote_attname,
+ * so we find such pairs by doing a merge join.
  *
  * Returns true if every entry in it has a match, and false if not.
  */
@@ -6271,8 +6271,8 @@ match_attrmap(PGresult *res,
 		row += 1;
 
 		/*
-		 * If the attname in the entry is less than that in the next stats
-		 * row, it means the stats for the entry are missing.
+		 * If the remote_attname in the entry is less than that in the next
+		 * stats row, it means the stats for the entry are missing.
 		 */
 		if (strcmp(remattrmap[mapidx].remote_attname,
 				   PQgetvalue(res, row, ATTSTATS_ATTNAME)) < 0)
@@ -6304,7 +6304,7 @@ match_attrmap(PGresult *res,
 }
 
 /*
- * Import fetched statistics into the local statistics tables.
+ * Attempt to import fetched statistics into the local statistics tables.
  */
 static bool
 import_fetched_statistics(Relation relation,
@@ -6314,6 +6314,12 @@ import_fetched_statistics(Relation relation,
 						  const RemoteAttributeMapping *remattrmap,
 						  int attrcnt)
 {
+	/*
+	 * ATTSTATS_NUM_FIELDS corresponds to the number of NullableDatum
+	 * arguments for import_attribute_statistics(), which is larger than that
+	 * for import_relation_statistics().  Therefore, a NullableDatum array of
+	 * that length is used to store data for each argument in both functions.
+	 */
 	NullableDatum args[ATTSTATS_NUM_FIELDS];
 
 	/* Set the 'version' parameter, which is common to both statistics. */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`strcmp`
- 大写宏：`ATTSTATS_ATTNAME`
- 大写宏：`ATTSTATS_NUM_FIELDS`
- 外部类型：`As`
- 外部类型：`AttrNumber`
- 外部类型：`If`
- 外部类型：`Import`
- 外部类型：`Match`
- 外部类型：`NullableDatum`
- 外部类型：`Relation`
- 外部类型：`RemoteAttributeMapping`
- 外部类型：`Returns`
- 外部类型：`Set`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f7c162fe3a` → 本草稿移入 `cases/defect/auto-postgres-f7c162fe3a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
