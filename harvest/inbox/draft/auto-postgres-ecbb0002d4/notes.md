# auto-postgres-ecbb0002d4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#dca6a9e320e0272f0ea8d7e076cda04b06050866](https://github.com/postgres/postgres/commit/dca6a9e320e0272f0ea8d7e076cda04b06050866) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #dca6a9e320e0272f0ea8d7e076cda04b06050866 (https://github.com/postgres/postgres/commit/dca6a9e320e0272f0ea8d7e076cda04b06050866)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2023；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT dca6a9e320e0272f0ea8d7e076cda04b06050866 Prevent self-join elimination when RTEs' checkAsUser fields differ. :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -37,23 +37,27 @@
 #include "optimizer/prep.h"
 #include "optimizer/restrictinfo.h"
 #include "parser/parse_agg.h"
+#include "parser/parse_relation.h"
 #include "rewrite/rewriteManip.h"
 #include "utils/lsyscache.h"
 
 /*
  * Utility structure.  A sorting procedure is needed to simplify the search
- * of SJE-candidate baserels referencing the same database relation.  Having
- * collected all baserels from the query jointree, the planner sorts them
- * according to the reloid value, groups them with the next pass and attempts
- * to remove self-joins.
- *
- * Preliminary sorting prevents quadratic behavior that can be harmful in the
- * case of numerous joins.
+ * for SJE-candidate baserels, which must reference the same database relation
+ * with the same reader permissions (checkAsUser value).  We require the
+ * checkAsUser fields to match to ensure that merged RTEs carry the same
+ * securityQuals; in future this rule might keep us out of trouble with other
+ * role-based features, too.  Having collected all baserels from the jointree,
+ * remove_self_joins_recurse sorts them according to their reloid and useroid
+ * values, groups them in another pass and attempts to remove self-joins
+ * within each group.  This preliminary sorting prevents quadratic behavior
+ * in the case of numerous joins.
  */
 typedef struct
 {
 	int			relid;
 	Oid			reloid;
+	Oid			useroid;
 } SelfJoinCandidate;
 
 bool		enable_self_join_elimination;
@@ -2011,24 +2015,35 @@ remove_self_joins_recurse(PlannerInfo *root, List *joinlist)
 		return removed;			/* ... but don't fail to report sub-removals */
 
 	/*
-	 * In order to find relations with the same oid we first build an array of
-	 * candidates and then sort it by oid.
+	 * In order to find relations with the same reloid/useroid we first build
+	 * an array of candidates and then sort it by those oids.
 	 */
 	candidates = palloc_array(SelfJoinCandidate, numRels);
 	i = -1;
 	j = 0;
 	while ((i = bms_next_member(relids, i)) >= 0)
 	{
+		RangeTblEntry *rte = root->simple_rte_array[i];
+
 		candidates[j].relid = i;
-		candidates[j].reloid = root->simple_rte_array[i]->relid;
+		candidates[j].reloid = rte->relid;
+		if (rte->perminfoindex != 0)
+		{
+			RTEPermissionInfo *perminfo;
+
+			perminfo = getRTEPermissionInfo(root->parse->rteperminfos, rte);
+			candidates[j].useroid = perminfo->checkAsUser;
+		}
+		else
+			candidates[j].useroid = InvalidOid;
 		j++;
 	}
 
 	qsort(candidates, numRels, sizeof(SelfJoinCandidate),
 		  self_join_candidates_cmp);
 
 	/*
-	 * Iteratively form a group of relation indexes with the same oid and
+	 * Iteratively form a group of relation indexes with the same oids and
 	 * launch the routine that detects self-joins in this group.
 	 *
 	 * We remove considered relations from relids as we scan, so that that set
@@ -2037,11 +2052,13 @@ remove_self_joins_recurse(PlannerInfo *root, List *joinlist)
 	i = 0;
 	for (j = 1; j <= numRels; j++)
 	{
-		if (j == numRels || candidates[j].reloid != candidates[i].reloid)
+		if (j == numRels ||
+			candidates[j].reloid != candidates[i].reloid ||
+			candidates[j].useroid != candidates[i].useroid)
 		{
 			if (j - i >= 2)
 			{
-				/* Create a group of relation indexes with the same oid */
+				/* Create a group of relation indexes with the same oids */
 				Relids		group = NULL;
 
 				while (i < j)
@@ -2073,7 +2090,7 @@ remove_self_joins_recurse(PlannerInfo *root, List *joinlist)
 }
 
 /*
- * Compare self-join candidates by their oids.
+ * Compare self-join candidates by their reloid and then useroid.
  */
 static int
 self_join_candidates_cmp(const void *a, const void *b)
@@ -2083,6 +2100,8 @@ self_join_candidates_cmp(const void *a, const void *b)
 
 	if (ca->reloid != cb->reloid)
 		return (ca->reloid < cb->reloid ? -1 : 1);
+	else if (ca->useroid != cb->useroid)
+		return (ca->useroid < cb->useroid ? -1 : 1);
 	else
 		return 0;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_next_member`
- 外部函数：`palloc_array`
- 外部函数：`qsort`
- 大写宏：`NULL`
- 大写宏：`SJE`
- 外部类型：`Compare`
- 外部类型：`Create`
- 外部类型：`Having`
- 外部类型：`In`
- 外部类型：`Iteratively`
- 外部类型：`Oid`
- 外部类型：`Preliminary`
- 外部类型：`Relids`
- 外部类型：`SelfJoinCandidate`
- 外部类型：`Utility`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ecbb0002d4` → 本草稿移入 `cases/defect/auto-postgres-ecbb0002d4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
