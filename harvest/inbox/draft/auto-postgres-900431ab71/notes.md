# auto-postgres-900431ab71

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#206e71935e1ed86e6116b118d7b77e78efcff8b8](https://github.com/postgres/postgres/commit/206e71935e1ed86e6116b118d7b77e78efcff8b8) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #206e71935e1ed86e6116b118d7b77e78efcff8b8 (https://github.com/postgres/postgres/commit/206e71935e1ed86e6116b118d7b77e78efcff8b8)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 18（原始 PR diff 行 1381；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 206e71935e1ed86e6116b118d7b77e78efcff8b8 Exclude new pages from VACUUM reltuples estimates :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1352,37 +1352,51 @@ vacuum_xid_failsafe_check(const struct VacuumCutoffs *cutoffs)
  *		the old value of pg_class.reltuples/pg_class.relpages as a measurement
  *		of the tuple density in the unscanned pages.
  *
+ *		scanned_new_pages is the number of scanned PageIsNew blocks. Such
+ *		blocks contain no tuples and are not a representative sample, so
+ *		exclude them from the sampled pages and sampleable population used to
+ *		extrapolate reltuples.
+ *
  *		Note: scanned_tuples should count only *live* tuples, since
  *		pg_class.reltuples is defined that way.
  */
 double
 vac_estimate_reltuples(Relation relation,
 					   BlockNumber total_pages,
 					   BlockNumber scanned_pages,
+					   BlockNumber scanned_new_pages,
 					   double scanned_tuples)
 {
 	BlockNumber old_rel_pages = relation->rd_rel->relpages;
+	BlockNumber sampled_pages;
+	BlockNumber sampleable_pages;
 	double		old_rel_tuples = relation->rd_rel->reltuples;
 	double		old_density;
 	double		unscanned_pages;
 	double		total_tuples;
 
+	Assert(scanned_new_pages <= scanned_pages);
+	Assert(scanned_new_pages <= total_pages);
+
 	/* If we did scan the whole table, just use the count as-is */
 	if (scanned_pages >= total_pages)
 		return scanned_tuples;
 
+	sampled_pages = scanned_pages - scanned_new_pages;
+	sampleable_pages = total_pages - scanned_new_pages;
+
 	/*
 	 * With no prior density, a tiny non-random sample is not sufficient to
 	 * estimate the relation's tuple count. This can happen when on-access
 	 * pruning marks most pages all-visible before the first VACUUM.
 	 */
 	if (old_rel_tuples < 0 || old_rel_pages == 0)
 	{
-		if (scanned_pages <= 1 ||
-			scanned_pages < (double) total_pages * 0.02)
+		if (sampled_pages <= 1 ||
+			sampled_pages < (double) sampleable_pages * 0.02)
 			return -1;
 
-		return floor((scanned_tuples / scanned_pages) * total_pages + 0.5);
+		return floor((scanned_tuples / sampled_pages) * sampleable_pages + 0.5);
 	}
 
 	/*
@@ -1398,12 +1412,10 @@ vac_estimate_reltuples(Relation relation,
 	 * value when at most one page was scanned.  When the relation size has
 	 * changed, use the old density to account for the added or removed pages
 	 * instead of preserving a stale tuple count.
-	 *
-	 * (Note: we might be returning -1 here.)
 	 */
 	if (old_rel_pages == total_pages &&
-		(scanned_pages <= 1 ||
-		 scanned_pages < (double) total_pages * 0.02))
+		(sampled_pages <= 1 ||
+		 sampled_pages < (double) sampleable_pages * 0.02))
 		return old_rel_tuples;
 
 	/*
@@ -1413,7 +1425,7 @@ vac_estimate_reltuples(Relation relation,
 	 * and finally add on the number of tuples in the scanned pages.
 	 */
 	old_density = old_rel_tuples / old_rel_pages;
-	unscanned_pages = (double) total_pages - (double) scanned_pages;
+	unscanned_pages = (double) sampleable_pages - (double) sampled_pages;
 	total_tuples = old_density * unscanned_pages + scanned_tuples;
 	return floor(total_tuples + 0.5);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`floor`
- 大写宏：`VACUUM`
- 外部类型：`BlockNumber`
- 外部类型：`If`
- 外部类型：`Note`
- 外部类型：`Relation`
- 外部类型：`This`
- 外部类型：`When`
- 外部类型：`With`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-900431ab71` → 本草稿移入 `cases/defect/auto-postgres-900431ab71/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
